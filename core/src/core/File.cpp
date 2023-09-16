#include "core/File.h"

namespace core
{
	Result<String> File::content(Allocator* allocator, StringView name)
	{
		auto f = File::open(allocator, name, IO_MODE_READ, OPEN_MODE_OPEN_ONLY);
		if (!f)
			return errf(allocator, "failed to open file '{}'"_sv, name);

		auto size = f->size();

		core::String res{allocator};
		res.resize(size);

		int64_t i = 0;
		while (i < size)
		{
			auto read_size = f->read(res.data() + i, size - i);
			if (read_size == 0)
				break;
			i += read_size;
		}

		if (i < size)
			return errf(allocator, "failed to read file '{}' completely"_sv, name);

		return res;
	}
}