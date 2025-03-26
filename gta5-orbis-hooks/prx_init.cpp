#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <kernel.h>

void NopCode(uint64_t address, size_t nopCount)
{
	sceKernelMprotect(reinterpret_cast<const void*>(address), nopCount, SCE_KERNEL_PROT_CPU_ALL);
	for (size_t i = 0; i < nopCount; i++)
	{
		*(uint8_t*)(address + i) = 0x90;
	}
}

class CScriptTextConstruction
{
public:
	// Subtitles
	static void BeginPrint(const char* pMainTextLabel);
	static void EndPrint(int32_t Duration, bool bPrintNow);

	// Text you can place anywhere
	static void BeginDisplayText(const char* pMainTextLabel)
	{
		reinterpret_cast<void(*)(const char*)>(0x010dfe40)(pMainTextLabel);
	}

	static void EndDisplayText(float DisplayAtX, float DisplayAtY, bool bForceUseDebugText, int Stereo = 0)
	{
		reinterpret_cast<void(*)(float, float, bool, int)>(0x010dff70)(DisplayAtX, DisplayAtY, bForceUseDebugText, Stereo);
	}

	static void AddSubStringLiteralString(const char* pSubStringLiteralStringToAdd)
	{
		reinterpret_cast<void(*)(const char*)>(0x010e1570)(pSubStringLiteralStringToAdd);
	}
};

void on_loop()
{
	CScriptTextConstruction::BeginDisplayText("STRING");
	CScriptTextConstruction::AddSubStringLiteralString("Hello, world!");
	CScriptTextConstruction::EndDisplayText(0.5f, 0.5f, false);
}

// Hook CTheScripts::Process so we can call functions from the game loop
void CTheScripts_Process_hook()
{
	// Call CTheScripts::InternalProcess, which is the only thing the original did.
	reinterpret_cast<void(*)(void)>(0x010c8650)();

	// Call our own function.
	on_loop();
}

int module_start(size_t args, const void* argp)
{
	printf("module_start(%zu, %p)\n", args, argp);

	printf("Built on %s at %s\n", __DATE__, __TIME__);

	// Disable the intro movie
	{
		sceKernelMprotect(reinterpret_cast<const void*>(0x2D91868), 2, SCE_KERNEL_PROT_CPU_ALL);
		*(uint16_t*)0x2D91868 = 0;
	}

	// Skip the legal screens
	{
		sceKernelMprotect(reinterpret_cast<const void*>(0x0046eb18), 6, SCE_KERNEL_PROT_CPU_ALL);
		*(uint32_t*)0x0046eb1a = 1;
	}

	// Disable Hang Detect Thread
	{
		NopCode(0x01e9a826, 5); // call sysIpcCreateThread
		NopCode(0x01e9a832, 7); // store to s_HangDetectThreadId
	}

	// Loading percentage patches
	// Display every percentage from 0 to 100, instead of only incrementing by 10
	{
		sceKernelMprotect(reinterpret_cast<const void*>(0x46F98E), 12, SCE_KERNEL_PROT_CPU_ALL);
		NopCode(0x46F98E, 12);
		// `mov r12d, ecx`
		*(uint8_t*)0x46F996 = 0x41;
		*(uint8_t*)0x46F997 = 0x89;
		*(uint8_t*)0x46F998 = 0xCC;

		// changes the cap from `99` to `100` (max)
		sceKernelMprotect(reinterpret_cast<const void*>(0x46F986), 3, SCE_KERNEL_PROT_CPU_ALL);
		*(uint8_t*)0x0046F988 = 100;
	}

	// Disable loading ExtraContent.
	{
		sceKernelMprotect(reinterpret_cast<const void*>(0xE658B0), 1, SCE_KERNEL_PROT_CPU_ALL);
		*(uint8_t*)0xE658B0 = 0xC3;
	}

	// Manually hook a function
	{
		sceKernelMprotect(reinterpret_cast<const void*>(0x010c8640), 12, SCE_KERNEL_PROT_CPU_ALL);
		*(uint16_t*)(0x010c8640 + 0) = 0xB848;
		*(uint64_t*)(0x010c8640 + 2) = (uint64_t)&CTheScripts_Process_hook;
		*(uint16_t*)(0x010c8640 + 10) = 0xE0FF;
		*(uint8_t*)(0x010c8640 + 12) = 0xC3; // Not needed, safety precaution.
	}

	return 0;
}

int module_stop(size_t args, const void* argp)
{
	printf("module_stop(%zu, %p)\n", args, argp);
	return 0;
}