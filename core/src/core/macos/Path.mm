#include "core/Path.h"

#import <Foundation/Foundation.h>

namespace core
{
	Result<String> Path::tmpDir(Allocator* allocator)
	{
		auto tmp = NSTemporaryDirectory();
		return String{StringView{[tmp UTFString]}, allocator};
	}
}