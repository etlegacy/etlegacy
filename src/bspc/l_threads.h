/*
 * Wolfenstein: Enemy Territory GPL Source Code
 * Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.
 *
 * ET: Legacy
 * Copyright (C) 2012 Jan Simek <jsimek.cz@gmail.com>
 *
 * This file is part of ET: Legacy.
 *
 * ET: Legacy is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * ET: Legacy is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ET: Legacy. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, Wolfenstein: Enemy Territory GPL Source Code is also
 * subject to certain additional terms. You should have received a copy
 * of these additional terms immediately following the terms and conditions
 * of the GNU General Public License which accompanied the source code.
 * If not, please request a copy in writing from id Software at the address below.
 *
 * id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.
 *
 * @file l_threads.h
 * @author Mr Elusive (MrElusive@demigod.demon.nl)
 * @brief multi-threading
 */

extern int numthreads;

void ThreadSetDefault( void );
int GetThreadWork( void );
void RunThreadsOnIndividual( int workcnt, qboolean showpacifier, void ( *func )( int ) );
void RunThreadsOn( int workcnt, qboolean showpacifier, void ( *func )( int ) );

//mutex
void ThreadSetupLock( void );
void ThreadShutdownLock( void );
void ThreadLock( void );
void ThreadUnlock( void );
//semaphore
void ThreadSetupSemaphore( void );
void ThreadShutdownSemaphore( void );
void ThreadSemaphoreWait( void );
void ThreadSemaphoreIncrease( int count );
//add/remove threads
void AddThread( void ( *func )( int ) );
void RemoveThread( int threadid );
void WaitForAllThreadsFinished( void );
int GetNumThreads( void );

