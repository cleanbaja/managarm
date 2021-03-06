#include <string.h>

#include "../common.hpp"
#include "zero.hpp"

#include <experimental/coroutine>

namespace {

struct ZeroFile final : File {
private:
	async::result<frg::expected<Error, size_t>>
	readSome(Process *, void *data, size_t length) override {
		memset(data, 0, length);
		co_return length;
	}

	async::result<frg::expected<Error, size_t>> writeAll(Process *, const void *, size_t length) override {
		co_return length;
	}

	async::result<frg::expected<Error, off_t>> seek(off_t, VfsSeek) override {
		co_return 0;
	}

	helix::BorrowedDescriptor getPassthroughLane() override {
		return _passthrough;
	}

	helix::UniqueLane _passthrough;
	async::cancellation_event _cancelServe;

public:
	static void serve(smarter::shared_ptr<ZeroFile> file) {
		helix::UniqueLane lane;
		std::tie(lane, file->_passthrough) = helix::createStream();
		async::detach(protocols::fs::servePassthrough(std::move(lane),
				file, &fileOperations, file->_cancelServe));
	}

	ZeroFile(std::shared_ptr<MountView> mount, std::shared_ptr<FsLink> link)
	: File{StructName::get("zero-file"), std::move(mount), std::move(link)} { }
};

struct ZeroDevice final : UnixDevice {
	ZeroDevice()
	: UnixDevice(VfsType::charDevice) {
		assignId({1, 5});
	}
	
	std::string nodePath() override {
		return "zero";
	}
	
	FutureMaybe<SharedFilePtr> open(std::shared_ptr<MountView> mount, std::shared_ptr<FsLink> link,
			SemanticFlags semantic_flags) override {
		assert(!(semantic_flags & ~(semanticRead | semanticWrite)));
		auto file = smarter::make_shared<ZeroFile>(std::move(mount), std::move(link));
		file->setupWeakFile(file);
		ZeroFile::serve(file);
		co_return File::constructHandle(std::move(file));
	}
};

} // anonymous namespace

std::shared_ptr<UnixDevice> createZeroDevice() {
	return std::make_shared<ZeroDevice>();
}
