#pragma once

extern "C" {
#include <nvidia-modeset-os-interface.h>
}

#include <DPC.h>

#include <util/DoublyLinkedList.h>


struct nvkms_timer_t;


class NvTimerQueue {
private:
	friend struct nvkms_timer_t;

	class TimerListItem: public DoublyLinkedListLinkImpl<TimerListItem> {};

	DPCQueue fDpcQueue;

	DoublyLinkedList<TimerListItem> fTimerList;

	spinlock fSpinlock = B_SPINLOCK_INITIALIZER;
	bool fClosing = false;

	// Timers can be allocated from an interrupt handler (nvkms asks for a timer with a delay of 0 from its vblank callback, to
	// move the work out of the interrupt). The heap takes a mutex, which must not be done with interrupts disabled, so such
	// timers come from a pool of preallocated slots. Both are guarded by fSpinlock.
	static const int kPoolSlots = 64;
	static const size_t kPoolSlotSize = 256;
	alignas(16) char fPool[kPoolSlots][kPoolSlotSize];
	uint64 fPoolBusy = 0;

	nvkms_timer_t *NewTimer(nvkms_timer_proc_t *proc, void *dataPtr, NvU32 dataU32, bool isRefPtr, bool kernelTimer);
	void DeleteTimer(nvkms_timer_t *timer);

public:
	NvTimerQueue();
	~NvTimerQueue();

	status_t Init();
	void Fini();

	nvkms_timer_handle_t *Alloc(nvkms_timer_proc_t *proc, void *dataPtr, NvU32 dataU32, NvU64 usec);
	NvBool AllocWithRefPtr(nvkms_timer_proc_t *proc, struct nvkms_ref_ptr *ref_ptr, NvU32 dataU32, NvU64 usec);
	void Free(nvkms_timer_handle_t *handle);
};
