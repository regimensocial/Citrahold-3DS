#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <3ds.h>
#include <citro2d.h>
#include <string>
#include <vector>
#include <iostream>
#include <tuple>

#include "types/menuItemsType.h"
#include "menuSystem.h"
#include "systemCore.h"

// #include <malloc.h>    // for mallinfo() 

int main()
{
	SystemCore systemCore;

	while (aptMainLoop())
	{
		
		systemCore.handleInput();
		systemCore.sceneRender();

		if (systemCore.isHalted())
			break;
	}

	systemCore.cleanExit();

	return 0;
}
