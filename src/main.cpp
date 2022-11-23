//#define _OLD_SYS_

#ifdef _OLD_SYS_
#include "vk_engine.h"
#else
#include "MApplication.h"
#endif

#include <iostream>

int main(int argc, char* argv[])
{

#ifdef _OLD_SYS_
	moo::VulkanEngine engine{};
#else
	moo::MApplication app{};
#endif
	
	try
	{
#ifdef _OLD_SYS_
		engine.init();	
		engine.run();	
		engine.cleanup();	
#else
		app.run();
#endif
	}
	catch(const std::exception& e) {
		std::cerr << e.what() << '\n';
		return EXIT_FAILURE;
	}

	return EXIT_SUCCESS;
}