/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 *                                                                           *
 *  Ghost, a micro-kernel based operating system for the x86 architecture    *
 *  Copyright (C) 2015, Max Schlüssel <lokoxe@gmail.com>                     *
 *                                                                           *
 *  This program is free software: you can redistribute it and/or modify     *
 *  it under the terms of the GNU General Public License as published by     *
 *  the Free Software Foundation, either version 3 of the License, or        *
 *  (at your option) any later version.                                      *
 *                                                                           *
 *  This program is distributed in the hope that it will be useful,          *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of           *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the            *
 *  GNU General Public License for more details.                             *
 *                                                                           *
 *  You should have received a copy of the GNU General Public License        *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.    *
 *                                                                           *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

#include "kernel/system/interrupts/interrupts.hpp"
#include "kernel/logger/logger.hpp"
#include "kernel/system/processor/processor.hpp"
#include "kernel/system/system.hpp"
#include "kernel/tasking/tasking.hpp"
#include "kernel/tasking/tasking_directory.hpp"
#include "kernel/tasking/scheduler/scheduler.hpp"
#include <ghost/memory/types.h>

namespace
{

void panicDumpCurrentTask()
{
	if(!systemIsReady())
		return;

	g_task* task = taskingGetCurrentTask();
	if(!task)
		return;

	const char* identifier = taskingDirectoryGetIdentifier(task->id);
	logInfo("%# current task: %i (%s) process=%i level=%i status=%i type=%i",
	        task->id,
	        identifier ? identifier : "anonymous",
	        task->process ? task->process->id : -1,
	        task->securityLevel,
	        task->status,
	        task->type);

	logInfo("%#   stack: %h - %h  intr: %h - %h",
	        task->stack.start, task->stack.end,
	        task->interruptStack.start, task->interruptStack.end);

	if(task->state)
	{
		logInfo("%#   last state: RIP=%h RSP=%h RFLAGS=%h",
		        task->state->rip, task->state->rsp, task->state->rflags);
	}
}

void panicDumpStackTrace()
{
	g_address rsp;
	g_address rbp;
	asm volatile("mov %%rsp, %0" : "=r"(rsp));
	asm volatile("mov %%rbp, %0" : "=r"(rbp));

	logInfo("%# panic context: RSP=%h RBP=%h", rsp, rbp);
	logInfo("%#   raw backtrace:");

	auto frame = reinterpret_cast<g_address*>(rbp);
	for(int depth = 0; depth < 24 && frame; ++depth)
	{
		if(frame[1] < 0x1000)
			break;
		logInfo("%#     [%02i] %h", depth, frame[1]);

		auto next = reinterpret_cast<g_address*>(frame[0]);
		if(!next || next <= frame)
			break;
		frame = next;
	}
}

} // namespace

void panic(const char* msg, ...)
{
	interruptsDisable();
	logInfo("%*%! unrecoverable error on processor %i", 0x0C, "kernerr", processorGetCurrentId());

	va_list valist;
	va_start(valist, msg);
	loggerPrintFormatted(msg, valist);
	va_end(valist);
	loggerPrintCharacter('\n');

	panicDumpCurrentTask();
	panicDumpStackTrace();
	if(systemIsReady())
		schedulerDump();

	for(;;)
		asm("hlt");
}
