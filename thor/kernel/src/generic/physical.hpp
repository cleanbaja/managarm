
namespace thor {

class PhysicalChunkAllocator {
public:
	typedef frigg::TicketLock Lock;
	typedef frigg::LockGuard<Lock> Guard;

	PhysicalChunkAllocator(PhysicalAddr bootstrap_base,
			size_t bootstrap_length);
	
	void bootstrap();

	PhysicalAddr allocate(Guard &guard, size_t size);
	void free(Guard &guard, PhysicalAddr address);

	size_t numUsedPages();
	size_t numFreePages();

	Lock lock;

private:
	PhysicalAddr _bootstrapBase;
	size_t _bootstrapLength;

	frigg::BuddyAllocator _buddy;

	size_t _usedPages;
	size_t _freePages;
};

} // namespace thor

