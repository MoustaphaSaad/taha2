#include <core/Mallocator.h>
#include <core/Log.h>
#include <core/SHA1.h>
#include <core/Path.h>
#include <core/MemoryStream.h>
#include <core/IPCMutex.h>
#include <core/Lock.h>

inline core::Result<core::String> uniquePathForFile(core::StringView view, core::Allocator* allocator)
{
	auto hash = core::SHA1::hash(view);
	auto tmpResult = core::Path::tmpDir(allocator);
	if (tmpResult.isError()) return tmpResult.releaseError();

	core::MemoryStream stream{allocator};
	core::strf(&stream, "budget_byte_"_sv);
	for (auto b: hash.asBytes())
		core::strf(&stream, "{:02x}"_sv, b);

	return core::Path::join(allocator, tmpResult.releaseValue(), stream.releaseString());
}

int main(int argc, char** argv)
{
	if (argc < 2)
		return EXIT_FAILURE;
	auto filename = core::StringView{argv[1]};

	core::Mallocator allocator{};
	core::Log log{&allocator};
	auto uniquePathResult = uniquePathForFile(filename, &allocator);
	if (uniquePathResult.isError())
	{
		log.critical("{}"_sv, uniquePathResult.releaseError());
		return EXIT_FAILURE;
	}
	auto uniquePath = uniquePathResult.releaseValue();
	log.info("file: {}, uniquePath: {}"_sv, filename, uniquePath);

	core::IPCMutex mutex{uniquePath, &allocator};
	auto lock = core::tryLockGuard(mutex);
	if (!lock.isLocked())
	{
		log.info("another instance already running"_sv);
		return EXIT_FAILURE;
	}

	// open an exclusive file
	return EXIT_SUCCESS;
}