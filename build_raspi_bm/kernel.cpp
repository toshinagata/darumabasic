//
// kernel.cpp
//
// Circle - A C++ bare metal environment for Raspberry Pi
// Copyright (C) 2014-2016  R. Stange <rsta2@o2online.de>
// 
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include "kernel.h"
#include <circle/usb/usbkeyboard.h>
#include <circle/string.h>
#include <circle/util.h>
#include <circle/timer.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>

#include "circle_bind.h"
#include "screen.h"

static const char FromKernel[] = "kernel";

CKernel *CKernel::s_pThis = 0;

CKernel::CKernel (void)
:	m_Screen (m_Options.GetWidth (), m_Options.GetHeight ()),
	m_Timer (&m_Interrupt),
	m_Logger (LogError, &m_Timer),
	m_DWHCI (&m_Interrupt, &m_Timer),
	m_ucLEDStatus (0xFF),
	m_ShutdownMode (ShutdownNone),
	m_Lock(TRUE)
{
	
	s_pThis = this;

	my_graphic_mode = 1;

//	m_ActLED.Blink (5);	// show we are alive
}

CKernel::~CKernel (void)
{
	s_pThis = 0;
}

boolean CKernel::Initialize (void)
{
	boolean bOK = TRUE;

	if (bOK)
	{
		bOK = m_Screen.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Serial.Initialize (115200);
	}

	if (bOK)
	{
		CDevice *pTarget = m_DeviceNameService.GetDevice (m_Options.GetLogDevice (), FALSE);
		if (pTarget == 0)
		{
			pTarget = &m_Screen;
		}

		bOK = m_Logger.Initialize (pTarget);
	}

	if (bOK)
	{
		bOK = m_Interrupt.Initialize ();
	}

	if (bOK)
	{
		bOK = m_Timer.Initialize ();
	}

	if (bOK)
	{
		bOK = m_DWHCI.Initialize ();
	}

	memset((void *)m_KeyBuffer, 0, sizeof(m_KeyBuffer));
	m_KeyBufferCount = m_KeyBufferBase = 0;

	return bOK;
}

extern "C" int bs_runloop(void);

TShutdownMode CKernel::Run (void)
{
	m_Logger.Write (FromKernel, LogNotice, "Compile time: " __DATE__ " " __TIME__);

	CUSBKeyboardDevice *pKeyboard = (CUSBKeyboardDevice *) m_DeviceNameService.GetDevice ("ukbd1", FALSE);
	if (pKeyboard == 0)
	{
		m_Logger.Write (FromKernel, LogError, "Keyboard not found");
	} else {
		m_Logger.Write (FromKernel, LogNotice, "Just type something!");
	}
//	::bs_blink(5);

#if 1	// set to 0 to test raw mode
	pKeyboard->RegisterShutdownHandler (ShutdownHandler);
	pKeyboard->RegisterKeyPressedHandler (KeyPressedHandler);
#else
	pKeyboard->RegisterKeyStatusHandlerRaw (KeyStatusHandlerRaw);
#endif

	m_ShutdownMode = ShutdownHalt;
	bs_runloop();
	return m_ShutdownMode;
	
#if 0
	for (unsigned nCount = 0; m_ShutdownMode == ShutdownNone; nCount++)
	{
		// CUSBKeyboardDevice::SetLEDs() must not be called in interrupt context,
		// that's why this must be done here.
		u8 ucLEDStatus = pKeyboard->GetLEDStatus ();
		if (ucLEDStatus != m_ucLEDStatus)
		{
			m_ucLEDStatus = ucLEDStatus;
			if (!pKeyboard->SetLEDs (m_ucLEDStatus))
			{
				m_Logger.Write (FromKernel, LogError, "Cannot set LED status");
			}
		}

		m_Screen.Rotor (0, nCount);
		m_Timer.MsDelay (100);
	}

	return m_ShutdownMode;
#endif
}

int CKernel::GetKey(void)
{
	int n;
//	m_Lock.Acquire();
	if (m_KeyBufferCount > 0) {
		n = m_KeyBuffer[m_KeyBufferBase++];
		m_KeyBufferBase %= sizeof(m_KeyBuffer);
		m_KeyBufferCount--;
	} else n = -1;
//	m_Lock.Release();
	if (n == '\n')
		n = '\r';  /*  Fix circle's misbehavior (gives '\n' for Return key)  */
	return n;
}

void CKernel::KeyPressedHandlerFunc(const char *pString)
{
	int i;
//	m_Lock.Acquire();
	for (i = 0; pString[i] != 0; i++) {
		if (m_KeyBufferCount < (int)sizeof(m_KeyBuffer)) {
			m_KeyBuffer[(m_KeyBufferBase + m_KeyBufferCount) % sizeof(m_KeyBuffer)] = pString[i];
			m_KeyBufferCount++;
		}
	}
//	m_Lock.Release();
}

void CKernel::KeyPressedHandler (const char *pString)
{
	int i;
	assert (s_pThis != 0);
	s_pThis->KeyPressedHandlerFunc(pString);
}

void CKernel::ShutdownHandler (void)
{
	assert (s_pThis != 0);
	s_pThis->m_ShutdownMode = ShutdownReboot;
}

void CKernel::KeyStatusHandlerRaw (unsigned char ucModifiers, const unsigned char RawKeys[6])
{
	assert (s_pThis != 0);

	CString Message;
	Message.Format ("Key status (modifiers %02X)", (unsigned) ucModifiers);

	for (unsigned i = 0; i < 6; i++)
	{
		if (RawKeys[i] != 0)
		{
			CString KeyCode;
			KeyCode.Format (" %02X", (unsigned) RawKeys[i]);

			Message.Append (KeyCode);
		}
	}

	s_pThis->m_Logger.Write (FromKernel, LogNotice, Message);
}

#if 0
#pragma mark ====== Interface with plain C ======
#endif

#include "circle_bind.h"
#include "daruma.h"
#include "screen.h"
#include <circle/alloc.h>

extern "C" {
	extern void *calloc(size_t count, size_t size);
	extern void *realloc(void *ptr, size_t size);
}

static void *s_circle_framebuffer = 0;

void *
bs_malloc(size_t size)
{
	return malloc(size);
}

void
bs_free(void *ptr)
{
	free(ptr);
}

void *
bs_calloc(size_t count, size_t size)
{
	return calloc(count, size);
}

void *
bs_realloc(void *ptr, size_t size)
{
	return realloc(ptr, size);
}

void
bs_raspibm_init_framebuffer(void)
{
	my_fb_width = CKernel::s_pThis->m_Screen.GetWidth();
	my_fb_height = CKernel::s_pThis->m_Screen.GetHeight();
	my_width = my_fb_width;
	my_height = my_fb_height;
	s_circle_framebuffer = CKernel::s_pThis->m_Screen.GetStatus().pContent;
	
}

void *
bs_raspibm_get_framebuffer(void)
{
	return s_circle_framebuffer;
}

void
bs_blink(int n)
{
	CKernel::s_pThis->m_ActLED.Blink (n);	// show we are alive
}

void
log_printf (const char *ptr, ...)
{
	va_list var;
	va_start (var, ptr);	
	CKernel::s_pThis->m_Logger.WriteV("emmc", LogNotice, ptr, var);
	va_end (var);
}

int
usleep(useconds_t useconds)
{
	CTimer::SimpleusDelay(useconds);
	return 0;
}

unsigned int
bs_start_kernel_timer(unsigned int nDelay, MyKernelTimerHandler *pHandler, void *pParam, void *pContext)
{
	return CTimer::Get()->StartKernelTimer(nDelay, pHandler, pParam, pContext);
}

void
bs_cancel_kernel_timer(unsigned int hTimer)
{
	CTimer::Get()->CancelKernelTimer(hTimer);
}

int
bs_getkey(void)
{
	int ch = CKernel::s_pThis->GetKey();
/*	if (ch > 0)
		log_printf("bs_getkey:%04x", ch); */
	return ch;
}
