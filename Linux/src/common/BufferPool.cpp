/*
* Copyright: JessMA Open Source (ldcsaa@gmail.com)
*
* Author	: Bruce Liang
* Website	: https://github.com/ldcsaa
* Project	: https://github.com/ldcsaa/HP-Socket
* Blog		: http://www.cnblogs.com/ldcsaa
* Wiki		: http://www.oschina.net/p/hp-socket
* QQ Group	: 44636872, 75375912
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "BufferPool.h"
#include "FuncHelper.h"

const DWORD TItem::DEFAULT_ITEM_CAPACITY			= DEFAULT_BUFFER_CACHE_CAPACITY;
const DWORD CBufferPool::DEFAULT_MAX_CACHE_SIZE		= 0;
const DWORD CBufferPool::DEFAULT_ITEM_CAPACITY		= CItemPool::DEFAULT_ITEM_CAPACITY;
const DWORD CBufferPool::DEFAULT_ITEM_POOL_SIZE		= CItemPool::DEFAULT_POOL_SIZE;
const DWORD CBufferPool::DEFAULT_ITEM_POOL_HOLD		= CItemPool::DEFAULT_POOL_HOLD;
const DWORD CBufferPool::DEFAULT_BUFFER_LOCK_TIME	= DEFAULT_OBJECT_CACHE_LOCK_TIME;
const DWORD CBufferPool::DEFAULT_BUFFER_POOL_SIZE	= DEFAULT_OBJECT_CACHE_POOL_SIZE;
const DWORD CBufferPool::DEFAULT_BUFFER_POOL_HOLD	= DEFAULT_OBJECT_CACHE_POOL_HOLD;

int TItem::Cat(const BYTE* pData, int length)
{
	ASSERT(pData != nullptr && length >= 0);

	int cat = MIN(Remain(), length);

	if(cat > 0)
	{
		memcpy(end, pData, cat);
		end += cat;
	}

	return cat;
}

int TItem::Cat(const TItem& other)
{
	ASSERT(this != &other);
	return Cat(other.Ptr(), other.Size());
}

int TItem::Fetch(BYTE* pData, int length)
{
	ASSERT(pData != nullptr && length > 0);

	int fetch = MIN(Size(), length);
	memcpy(pData, begin, fetch);
	begin	 += fetch;

	return fetch;
}

int TItem::Peek(BYTE* pData, int length)
{
	ASSERT(pData != nullptr && length > 0);

	int peek = MIN(Size(), length);
	memcpy(pData, begin, peek);

	return peek;
}

int TItem::Increase(int length)
{
	ASSERT(length >= 0);

	int increase = MIN(Remain(), length);
	end			+= increase;

	return increase;
}

int TItem::Reduce(int length)
{
	ASSERT(length >= 0);

	int reduce = MIN(Size(), length);
	begin	  += reduce;

	return reduce;
}

void TItem::Reset(int first, int last)
{
	ASSERT(first >= -1 && first <= capacity);
	ASSERT(last >= -1 && last <= capacity);

	if(first >= 0)	begin	= head + MIN(first, capacity);
	if(last >= 0)	end		= head + MIN(last, capacity);
}

TBuffer* TBuffer::Construct(CBufferPool& pool, ULONG_PTR dwID)
{
	ASSERT(dwID != 0);

	CPrivateHeap& heap	= pool.GetPrivateHeap();
	TBuffer* pBuffer	= (TBuffer*)heap.Alloc(sizeof(TBuffer));

	return ::ConstructObject(pBuffer, heap, pool.GetItemPool(), dwID);
}

void TBuffer::Destruct(TBuffer* pBuffer)
{
	ASSERT(pBuffer != nullptr);

	CPrivateHeap& heap = pBuffer->heap;
	::DestructObject(pBuffer);
	heap.Free(pBuffer);
}

void TBuffer::Reset()
{
	id		 = 0;
	length	 = 0;
	freeTime = ::TimeGetTime();
}

int TBuffer::Cat(const BYTE* pData, int len)
{
	items.Cat(pData, len);
	return IncreaseLength(len);
}

int TBuffer::Cat(const TItem* pItem)
{
	items.Cat(pItem);
	return IncreaseLength(pItem->Size());
}

int TBuffer::Cat(const TItemList& other)
{
	ASSERT(&items != &other);

	for(TItem* pItem = other.Front(); pItem != nullptr; pItem = pItem->next)
		Cat(pItem);

	return length;
}

int TBuffer::Fetch(BYTE* pData, int len)
{
	int fetch = items.Fetch(pData, len);
	DecreaseLength(fetch);

	return fetch;
}

int TBuffer::Peek(BYTE* pData, int len)
{
	return items.Peek(pData, len);
}

int TBuffer::Reduce(int len)
{
	int reduce = items.Reduce(len);
	DecreaseLength(reduce);

	return reduce;
}

void CBufferPool::PutFreeBuffer(ULONG_PTR dwID)
{
	ASSERT(dwID != 0);

	TBuffer* pBuffer = FindCacheBuffer(dwID);

	if(pBuffer != nullptr)
		PutFreeBuffer(pBuffer);
}

void CBufferPool::PutFreeBuffer(TBuffer* pBuffer)
{
	ASSERT(pBuffer != nullptr);

	if(!pBuffer->IsValid())
		return;

	m_bfCache.RemoveEx(pBuffer->ID());

	BOOL bOK = FALSE;

	{
		CCriSecLock locallock(pBuffer->cs);

		if(pBuffer->IsValid())
		{
			pBuffer->Reset();
			bOK = TRUE;
		}
	}

	if(bOK)
	{
		m_itPool.PutFreeItem(pBuffer->items);

#ifndef USE_EXTERNAL_GC
		ReleaseGCBuffer();
#endif
		if(!m_lsFreeBuffer.TryPut(pBuffer))
			m_lsGCBuffer.PushBack(pBuffer);
	}
}

void CBufferPool::ReleaseGCBuffer(BOOL bForce)
{
	::ReleaseGCObj(m_lsGCBuffer, m_dwBufferLockTime, bForce);
}

TBuffer* CBufferPool::PutCacheBuffer(ULONG_PTR dwID)
{
	ASSERT(dwID != 0);

	TBuffer* pBuffer = PickFreeBuffer(dwID);
	m_bfCache.SetEx(dwID, pBuffer);

	return pBuffer;
}

TBuffer* CBufferPool::PickFreeBuffer(ULONG_PTR dwID)
{
	ASSERT( dwID != 0);

	DWORD dwIndex;
	TBuffer* pBuffer = nullptr;

	if(m_lsFreeBuffer.TryLock(&pBuffer, dwIndex))
	{
		if(::GetTimeGap32(pBuffer->freeTime) >= m_dwBufferLockTime)
			VERIFY(m_lsFreeBuffer.ReleaseLock(nullptr, dwIndex));
		else
		{
			VERIFY(m_lsFreeBuffer.ReleaseLock(pBuffer, dwIndex));
			pBuffer = nullptr;
		}
	}

	if(pBuffer)	pBuffer->id	= dwID;
	else		pBuffer		= TBuffer::Construct(*this, dwID);

	ASSERT(pBuffer);
	return pBuffer;
}

TBuffer* CBufferPool::FindCacheBuffer(ULONG_PTR dwID)
{
	ASSERT(dwID != 0);

	TBuffer* pBuffer = nullptr;

	if(m_bfCache.GetEx(dwID, &pBuffer) != TBufferCache::GR_VALID)
		pBuffer = nullptr;

	return pBuffer;
}

void CBufferPool::Prepare()
{
	m_itPool.Prepare();

	m_bfCache.Reset(m_dwMaxCacheSize);
	m_lsFreeBuffer.Reset(m_dwBufferPoolSize);
}

void CBufferPool::Clear()
{
	TBufferCache::IndexSet& indexes = m_bfCache.Indexes();

	for(auto it = indexes.begin(), end = indexes.end(); it != end; ++it)
	{
		TBuffer* pBuffer = FindCacheBuffer(*it);
		if(pBuffer) TBuffer::Destruct(pBuffer);
	}

	m_bfCache.Reset();

	m_lsFreeBuffer.Clear();

	ReleaseGCBuffer(TRUE);
	VERIFY(m_lsGCBuffer.IsEmpty());

	m_itPool.Clear();
	m_heap.Reset();
}
