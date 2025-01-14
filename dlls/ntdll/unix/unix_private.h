/*
 * Ntdll Unix private interface
 *
 * Copyright (C) 2020 Alexandre Julliard
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#ifndef __NTDLL_UNIX_PRIVATE_H
#define __NTDLL_UNIX_PRIVATE_H

#include <pthread.h>
#include <signal.h>
#include "unixlib.h"
#include "wine/unixlib.h"
#include "wine/server.h"
#define WINE_LIST_HOSTADDRSPACE
#include "wine/list.h"
#include "wine/debug.h"

struct msghdr;

#if defined(__i386__) || defined(__i386_on_x86_64__)
static const WORD current_machine = IMAGE_FILE_MACHINE_I386;
#elif defined(__x86_64__)
static const WORD current_machine = IMAGE_FILE_MACHINE_AMD64;
#elif defined(__arm__)
static const WORD current_machine = IMAGE_FILE_MACHINE_ARMNT;
#elif defined(__aarch64__)
static const WORD current_machine = IMAGE_FILE_MACHINE_ARM64;
#endif
extern WORD native_machine DECLSPEC_HIDDEN;

static const BOOL is_win64 = (sizeof(void *) > sizeof(int));

static inline BOOL is_machine_64bit( WORD machine )
{
    return (machine == IMAGE_FILE_MACHINE_AMD64 || machine == IMAGE_FILE_MACHINE_ARM64);
}

/* thread private data, stored in NtCurrentTeb()->GdiTebBatch */
struct ntdll_thread_data
{
    void              *cpu_data[16];  /* reserved for CPU-specific data */
    void              *kernel_stack;  /* stack for thread startup and kernel syscalls */
    int                request_fd;    /* fd for sending server requests */
    int                reply_fd;      /* fd for receiving server replies */
    int                wait_fd[2];    /* fd for sleeping server requests */
    pthread_t          pthread_id;    /* pthread thread id */
    struct list        entry;         /* entry in TEB list */
    PRTL_THREAD_START_ROUTINE start;  /* thread entry point */
    void              *param;         /* thread entry point parameter */
    void              *jmp_buf;       /* setjmp buffer for exception handling */
    int                esync_apc_fd;  /* fd to wait on for user APCs */
};

C_ASSERT( sizeof(struct ntdll_thread_data) <= sizeof(((TEB *)0)->GdiTebBatch) );

static inline struct ntdll_thread_data *ntdll_get_thread_data(void)
{
    return (struct ntdll_thread_data *)&NtCurrentTeb()->GdiTebBatch;
}

/* returns TRUE if the async is complete; FALSE if it should be restarted */
typedef BOOL async_callback_t( void *user, ULONG_PTR *info, NTSTATUS *status );

struct async_fileio
{
    async_callback_t    *callback;
    struct async_fileio *next;
    HANDLE               handle;
};

static const SIZE_T page_size = 0x1000;
static const SIZE_T teb_size = 0x3800;  /* TEB64 + TEB32 + debug info */
static const SIZE_T signal_stack_mask = 0xffff;
static const SIZE_T signal_stack_size = 0x10000 - 0x3800;
static const SIZE_T kernel_stack_size = 0x20000;
static const SIZE_T min_kernel_stack  = 0x2000;
static const LONG teb_offset = 0x2000;

#define FILE_WRITE_TO_END_OF_FILE      ((LONGLONG)-1)
#define FILE_USE_FILE_POINTER_POSITION ((LONGLONG)-2)

/* callbacks to PE ntdll from the Unix side */
extern void     (WINAPI *pDbgUiRemoteBreakin)( void *arg ) DECLSPEC_HIDDEN;
extern NTSTATUS (WINAPI *pKiRaiseUserExceptionDispatcher)(void) DECLSPEC_HIDDEN;
extern NTSTATUS (WINAPI *pKiUserExceptionDispatcher)(EXCEPTION_RECORD*,CONTEXT*) DECLSPEC_HIDDEN;
extern void     (WINAPI *pKiUserApcDispatcher)(CONTEXT*,ULONG_PTR,ULONG_PTR,ULONG_PTR,PNTAPCFUNC) DECLSPEC_HIDDEN;
extern NTSTATUS (WINAPI *pKiUserExceptionDispatcher)(EXCEPTION_RECORD*,CONTEXT*) DECLSPEC_HIDDEN;
extern void     (WINAPI *pLdrInitializeThunk)(CONTEXT*,void**,ULONG_PTR,ULONG_PTR) DECLSPEC_HIDDEN;
extern void     (WINAPI *pRtlUserThreadStart)( PRTL_THREAD_START_ROUTINE entry, void *arg ) DECLSPEC_HIDDEN;
extern void *   (WINAPI *pRtlAllocateHeap)( HANDLE heap, ULONG flags, SIZE_T size ) DECLSPEC_HIDDEN;
extern BOOLEAN  (WINAPI *pRtlFreeHeap)( HANDLE heap, ULONG flags, void *ptr ) DECLSPEC_HIDDEN;
extern void     (WINAPI *pKiUserCallbackDispatcher)(ULONG,void*,ULONG) DECLSPEC_HIDDEN;
extern void     (WINAPI *pLdrInitializeThunk)(CONTEXT*,void**,ULONG_PTR,ULONG_PTR) DECLSPEC_HIDDEN;
extern void     (WINAPI *pRtlUserThreadStart)( PRTL_THREAD_START_ROUTINE entry, void *arg ) DECLSPEC_HIDDEN;
extern void     (WINAPI *p__wine_ctrl_routine)(void *) DECLSPEC_HIDDEN;
extern SYSTEM_DLL_INIT_BLOCK *pLdrSystemDllInitBlock DECLSPEC_HIDDEN;
extern LONGLONG CDECL fast_RtlGetSystemTimePrecise(void) DECLSPEC_HIDDEN;

extern NTSTATUS CDECL unwind_builtin_dll( ULONG type, struct _DISPATCHER_CONTEXT *dispatch,
                                          CONTEXT *context ) DECLSPEC_HIDDEN;

struct _FILE_FS_DEVICE_INFORMATION;

extern const char wine_build[] DECLSPEC_HIDDEN;

extern const char * HOSTPTR home_dir DECLSPEC_HIDDEN;
extern const char * HOSTPTR data_dir DECLSPEC_HIDDEN;
extern const char * HOSTPTR build_dir DECLSPEC_HIDDEN;
extern const char * HOSTPTR config_dir DECLSPEC_HIDDEN;
extern const char * HOSTPTR user_name DECLSPEC_HIDDEN;
extern const char * HOSTPTR * HOSTPTR dll_paths DECLSPEC_HIDDEN;
extern const char * HOSTPTR * HOSTPTR system_dll_paths DECLSPEC_HIDDEN;
extern PEB *peb DECLSPEC_HIDDEN;
extern SIZE_T startup_info_size DECLSPEC_HIDDEN;
extern BOOL is_prefix_bootstrap DECLSPEC_HIDDEN;
extern SECTION_IMAGE_INFORMATION main_image_info DECLSPEC_HIDDEN;
extern int main_argc DECLSPEC_HIDDEN;
extern char * HOSTPTR * HOSTPTR main_argv DECLSPEC_HIDDEN;
extern char * HOSTPTR * HOSTPTR main_envp DECLSPEC_HIDDEN;
extern WCHAR * HOSTPTR * HOSTPTR main_wargv DECLSPEC_HIDDEN;
extern const WCHAR system_dir[] DECLSPEC_HIDDEN;
extern unsigned int supported_machines_count DECLSPEC_HIDDEN;
extern USHORT supported_machines[8] DECLSPEC_HIDDEN;
extern BOOL process_exiting DECLSPEC_HIDDEN;
extern HANDLE keyed_event DECLSPEC_HIDDEN;
extern timeout_t server_start_time DECLSPEC_HIDDEN;
extern sigset_t server_block_set DECLSPEC_HIDDEN;
extern struct _KUSER_SHARED_DATA *user_shared_data DECLSPEC_HIDDEN;
extern SYSTEM_CPU_INFORMATION cpu_info DECLSPEC_HIDDEN;
#ifndef _WIN64
extern BOOL is_wow64 DECLSPEC_HIDDEN;
#endif
#if defined(__i386__) || defined(__i386_on_x86_64__)
extern struct ldt_copy __wine_ldt_copy DECLSPEC_HIDDEN;
#endif

#ifdef __i386_on_x86_64__
extern unsigned short wine_32on64_cs32;
extern unsigned short wine_32on64_cs64;
extern unsigned short wine_32on64_ds32;
#endif
extern int wine_needs_32on64(void);

extern void init_environment( int argc, char * HOSTPTR * HOSTPTR argv, char * HOSTPTR * HOSTPTR envp ) DECLSPEC_HIDDEN;
extern DWORD ntdll_umbstowcs( const char * HOSTPTR src, DWORD srclen, WCHAR * HOSTPTR dst, DWORD dstlen ) DECLSPEC_HIDDEN;
extern int ntdll_wcstoumbs( const WCHAR * HOSTPTR src, DWORD srclen, char * HOSTPTR dst, DWORD dstlen, BOOL strict ) DECLSPEC_HIDDEN;
extern void init_startup_info(void) DECLSPEC_HIDDEN;
extern void *create_startup_info( const UNICODE_STRING *nt_image, const RTL_USER_PROCESS_PARAMETERS *params,
                                  DWORD *info_size ) DECLSPEC_HIDDEN;
extern char * HOSTPTR * HOSTPTR build_envp( const WCHAR *envW ) DECLSPEC_HIDDEN;
extern NTSTATUS exec_wineloader( char * HOSTPTR * HOSTPTR argv, int socketfd, const pe_image_info_t *pe_info ) DECLSPEC_HIDDEN;
extern NTSTATUS load_builtin( const pe_image_info_t *image_info, WCHAR *filename, void **module, SIZE_T *size ) DECLSPEC_HIDDEN;
extern BOOL is_builtin_path( const UNICODE_STRING *path, WORD *machine ) DECLSPEC_HIDDEN;
extern NTSTATUS load_main_exe( const WCHAR * HOSTPTR dos_name, const char * HOSTPTR unix_name, const WCHAR * HOSTPTR curdir, WCHAR * HOSTPTR * HOSTPTR image, void **module ) DECLSPEC_HIDDEN;
extern NTSTATUS load_start_exe( WCHAR * HOSTPTR * HOSTPTR image, void **module ) DECLSPEC_HIDDEN;
extern void start_server( BOOL debug ) DECLSPEC_HIDDEN;

extern unsigned int server_call_unlocked( void *req_ptr ) DECLSPEC_HIDDEN;
extern void server_enter_uninterrupted_section( pthread_mutex_t *mutex, sigset_t *sigset ) DECLSPEC_HIDDEN;
extern void server_leave_uninterrupted_section( pthread_mutex_t *mutex, sigset_t *sigset ) DECLSPEC_HIDDEN;
extern unsigned int server_select( const select_op_t *select_op, data_size_t size, UINT flags,
                                   timeout_t abs_timeout, context_t *context, user_apc_t *user_apc ) DECLSPEC_HIDDEN;
extern unsigned int server_wait( const select_op_t *select_op, data_size_t size, UINT flags,
                                 const LARGE_INTEGER *timeout ) DECLSPEC_HIDDEN;
extern unsigned int server_queue_process_apc( HANDLE process, const apc_call_t *call,
                                              apc_result_t *result ) DECLSPEC_HIDDEN;
extern int server_get_unix_fd( HANDLE handle, unsigned int wanted_access, int *unix_fd,
                               int *needs_close, enum server_fd_type *type, unsigned int *options ) DECLSPEC_HIDDEN;
extern void wine_server_send_fd( int fd ) DECLSPEC_HIDDEN;
extern void process_exit_wrapper( int status ) DECLSPEC_HIDDEN;
extern size_t server_init_process(void) DECLSPEC_HIDDEN;
extern void server_init_process_done(void) DECLSPEC_HIDDEN;
extern void server_init_thread( void *entry_point, BOOL *suspend ) DECLSPEC_HIDDEN;
extern int server_pipe( int fd[2] ) DECLSPEC_HIDDEN;

extern void fpux_to_fpu( I386_FLOATING_SAVE_AREA *fpu, const XSAVE_FORMAT * HOSTPTR fpux ) DECLSPEC_HIDDEN;
extern void fpu_to_fpux( XSAVE_FORMAT *fpux, const I386_FLOATING_SAVE_AREA *fpu ) DECLSPEC_HIDDEN;
extern void *get_cpu_area( USHORT machine ) DECLSPEC_HIDDEN;
extern void set_thread_id( TEB *teb, DWORD pid, DWORD tid ) DECLSPEC_HIDDEN;
extern NTSTATUS init_thread_stack( TEB *teb, ULONG_PTR zero_bits, SIZE_T reserve_size, SIZE_T commit_size ) DECLSPEC_HIDDEN;
extern NTSTATUS context_to_server( context_t *to, USHORT to_machine, const void *src, USHORT from_machine ) DECLSPEC_HIDDEN;
extern NTSTATUS context_from_server( void *dst, const context_t *from, USHORT machine ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN abort_thread( int status ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN abort_process( int status ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN exit_process( int status ) DECLSPEC_HIDDEN;
extern void wait_suspend( CONTEXT *context ) DECLSPEC_HIDDEN;
extern NTSTATUS send_debug_event( EXCEPTION_RECORD *rec, CONTEXT *context, BOOL first_chance ) DECLSPEC_HIDDEN;
extern NTSTATUS set_thread_context( HANDLE handle, const void *context, BOOL *self, USHORT machine ) DECLSPEC_HIDDEN;
extern NTSTATUS get_thread_context( HANDLE handle, void *context, BOOL *self, USHORT machine ) DECLSPEC_HIDDEN;
extern NTSTATUS alloc_object_attributes( const OBJECT_ATTRIBUTES *attr, struct object_attributes * HOSTPTR * HOSTPTR ret,
                                         data_size_t *ret_len ) DECLSPEC_HIDDEN;

extern void * HOSTPTR anon_mmap_fixed( void * HOSTPTR start, size_t size, int prot, int flags ) DECLSPEC_HIDDEN;
extern void * HOSTPTR anon_mmap_alloc( size_t size, int prot ) DECLSPEC_HIDDEN;
extern void virtual_init(void) DECLSPEC_HIDDEN;
extern ULONG_PTR get_system_affinity_mask(void) DECLSPEC_HIDDEN;
extern void virtual_get_system_info( SYSTEM_BASIC_INFORMATION *info, BOOL wow64 ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_map_builtin_module( HANDLE mapping, void **module, SIZE_T *size,
                                            SECTION_IMAGE_INFORMATION *info, WORD machine, BOOL prefer_native ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_create_builtin_view( void *module, const UNICODE_STRING *nt_name,
                                             pe_image_info_t *info, void * HOSTPTR so_handle ) DECLSPEC_HIDDEN;
extern TEB *virtual_alloc_first_teb(void) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_alloc_teb( TEB **ret_teb ) DECLSPEC_HIDDEN;
extern void virtual_free_teb( TEB *teb ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_clear_tls_index( ULONG index ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_alloc_thread_stack( INITIAL_TEB *stack, ULONG_PTR zero_bits, SIZE_T reserve_size,
                                            SIZE_T commit_size, SIZE_T extra_size ) DECLSPEC_HIDDEN;
extern void virtual_map_user_shared_data(void) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_handle_fault( void * HOSTPTR addr, DWORD err, void *stack ) DECLSPEC_HIDDEN;
extern unsigned int virtual_locked_server_call( void *req_ptr ) DECLSPEC_HIDDEN;
extern ssize_t virtual_locked_read( int fd, void *addr, size_t size ) DECLSPEC_HIDDEN;
extern ssize_t virtual_locked_pread( int fd, void *addr, size_t size, off_t offset ) DECLSPEC_HIDDEN;
extern ssize_t virtual_locked_recvmsg( int fd, struct msghdr *hdr, int flags ) DECLSPEC_HIDDEN;
extern BOOL virtual_is_valid_code_address( const void *addr, SIZE_T size ) DECLSPEC_HIDDEN;
extern void *virtual_setup_exception( void *stack_ptr, size_t size, EXCEPTION_RECORD *rec ) DECLSPEC_HIDDEN;
extern BOOL virtual_check_buffer_for_read( const void *ptr, SIZE_T size ) DECLSPEC_HIDDEN;
extern BOOL virtual_check_buffer_for_write( void *ptr, SIZE_T size ) DECLSPEC_HIDDEN;
extern SIZE_T virtual_uninterrupted_read_memory( const void *addr, void *buffer, SIZE_T size ) DECLSPEC_HIDDEN;
extern NTSTATUS virtual_uninterrupted_write_memory( void *addr, const void *buffer, SIZE_T size ) DECLSPEC_HIDDEN;
extern void virtual_set_force_exec( BOOL enable ) DECLSPEC_HIDDEN;
extern void virtual_set_large_address_space(void) DECLSPEC_HIDDEN;
extern void virtual_fill_image_information( const pe_image_info_t *pe_info,
                                            SECTION_IMAGE_INFORMATION *info ) DECLSPEC_HIDDEN;
extern void release_builtin_module( void *module ) DECLSPEC_HIDDEN;
extern void * HOSTPTR get_builtin_so_handle( void *module ) DECLSPEC_HIDDEN;
extern NTSTATUS load_builtin_unixlib( void *module, const char * HOSTPTR name ) DECLSPEC_HIDDEN;

extern NTSTATUS get_thread_ldt_entry( HANDLE handle, void *data, ULONG len, ULONG *ret_len ) DECLSPEC_HIDDEN;
extern void *get_native_context( CONTEXT *context ) DECLSPEC_HIDDEN;
extern void *get_wow_context( CONTEXT *context ) DECLSPEC_HIDDEN;
extern BOOL get_thread_times( int unix_pid, int unix_tid, LARGE_INTEGER *kernel_time,
                              LARGE_INTEGER *user_time ) DECLSPEC_HIDDEN;
extern void signal_init_threading(void) DECLSPEC_HIDDEN;
extern NTSTATUS signal_alloc_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void signal_free_thread( TEB *teb ) DECLSPEC_HIDDEN;
#ifdef __i386_on_x86_64__
extern void signal_init_32on64_ldt(void) DECLSPEC_HIDDEN;
#endif
extern void signal_init_thread( TEB *teb ) DECLSPEC_HIDDEN;
extern void signal_init_process(void) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN signal_start_thread( PRTL_THREAD_START_ROUTINE entry, void *arg,
                                                   BOOL suspend, TEB *teb ) DECLSPEC_HIDDEN;
extern void DECLSPEC_NORETURN signal_exit_thread( int status, void (*func)(int), TEB *teb ) DECLSPEC_HIDDEN;
extern SYSTEM_SERVICE_TABLE KeServiceDescriptorTable[4] DECLSPEC_HIDDEN;
extern void __wine_syscall_dispatcher(void) DECLSPEC_HIDDEN;
extern void WINAPI DECLSPEC_NORETURN __wine_syscall_dispatcher_return( void *frame, ULONG_PTR retval ) DECLSPEC_HIDDEN;
extern NTSTATUS signal_set_full_context( CONTEXT *context ) DECLSPEC_HIDDEN;
extern NTSTATUS get_thread_wow64_context( HANDLE handle, void *ctx, ULONG size ) DECLSPEC_HIDDEN;
extern NTSTATUS set_thread_wow64_context( HANDLE handle, const void *ctx, ULONG size ) DECLSPEC_HIDDEN;
extern void fill_vm_counters( VM_COUNTERS_EX *pvmi, int unix_pid ) DECLSPEC_HIDDEN;
extern NTSTATUS open_hkcu_key( const char *path, HANDLE *key ) DECLSPEC_HIDDEN;

extern NTSTATUS cdrom_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                       IO_STATUS_BLOCK *io, ULONG code, void *in_buffer,
                                       ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;
extern NTSTATUS serial_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                        IO_STATUS_BLOCK *io, ULONG code, void *in_buffer,
                                        ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;
extern NTSTATUS serial_FlushBuffersFile( int fd ) DECLSPEC_HIDDEN;
extern NTSTATUS sock_ioctl( HANDLE handle, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user, IO_STATUS_BLOCK *io,
                            ULONG code, void *in_buffer, ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;
extern NTSTATUS tape_DeviceIoControl( HANDLE device, HANDLE event, PIO_APC_ROUTINE apc, void *apc_user,
                                      IO_STATUS_BLOCK *io, ULONG code, void *in_buffer,
                                      ULONG in_size, void *out_buffer, ULONG out_size ) DECLSPEC_HIDDEN;

extern struct async_fileio *alloc_fileio( DWORD size, async_callback_t callback, HANDLE handle ) DECLSPEC_HIDDEN;
extern void release_fileio( struct async_fileio *io ) DECLSPEC_HIDDEN;
extern NTSTATUS errno_to_status( int err ) DECLSPEC_HIDDEN;
extern BOOL get_redirect( OBJECT_ATTRIBUTES *attr, UNICODE_STRING *redir ) DECLSPEC_HIDDEN;
extern NTSTATUS nt_to_unix_file_name( const OBJECT_ATTRIBUTES *attr, char * HOSTPTR *name_ret, UINT disposition ) DECLSPEC_HIDDEN;
extern NTSTATUS unix_to_nt_file_name( const char * HOSTPTR name, WCHAR * HOSTPTR * HOSTPTR nt ) DECLSPEC_HIDDEN;
extern NTSTATUS get_full_path( const WCHAR * HOSTPTR name, const WCHAR * HOSTPTR curdir, WCHAR * HOSTPTR * HOSTPTR path ) DECLSPEC_HIDDEN;
extern NTSTATUS open_unix_file( HANDLE *handle, const char * HOSTPTR unix_name, ACCESS_MASK access,
                                OBJECT_ATTRIBUTES *attr, ULONG attributes, ULONG sharing, ULONG disposition,
                                ULONG options, void *ea_buffer, ULONG ea_length ) DECLSPEC_HIDDEN;
extern NTSTATUS get_device_info( int fd, struct _FILE_FS_DEVICE_INFORMATION *info ) DECLSPEC_HIDDEN;
extern void init_files(void) DECLSPEC_HIDDEN;
extern void init_cpu_info(void) DECLSPEC_HIDDEN;
extern void add_completion( HANDLE handle, ULONG_PTR value, NTSTATUS status, ULONG info, BOOL async ) DECLSPEC_HIDDEN;
extern void set_async_direct_result( HANDLE *optional_handle, NTSTATUS status, ULONG_PTR information );

extern void dbg_init(void) DECLSPEC_HIDDEN;

extern void WINAPI DECLSPEC_NORETURN call_user_apc_dispatcher( CONTEXT *context_ptr, ULONG_PTR ctx, ULONG_PTR arg1, ULONG_PTR arg2, PNTAPCFUNC func,
void (WINAPI *dispatcher)(CONTEXT*,ULONG_PTR,ULONG_PTR,ULONG_PTR,PNTAPCFUNC)) DECLSPEC_HIDDEN;
extern void WINAPI DECLSPEC_NORETURN call_user_exception_dispatcher( EXCEPTION_RECORD *rec, CONTEXT *context, NTSTATUS (WINAPI *dispatcher)(EXCEPTION_RECORD*,CONTEXT*) ) DECLSPEC_HIDDEN;
extern void WINAPI call_raise_user_exception_dispatcher( NTSTATUS (WINAPI *dispatcher)(void) ) DECLSPEC_HIDDEN;

#define IMAGE_DLLCHARACTERISTICS_PREFER_NATIVE 0x0010 /* Wine extension */

#define TICKSPERSEC 10000000
#define SECS_1601_TO_1970  ((369 * 365 + 89) * (ULONGLONG)86400)

static inline ULONGLONG ticks_from_time_t( time_t time )
{
    if (sizeof(time_t) == sizeof(int))  /* time_t may be signed */
        return ((ULONGLONG)(ULONG)time + SECS_1601_TO_1970) * TICKSPERSEC;
    else
        return ((ULONGLONG)time + SECS_1601_TO_1970) * TICKSPERSEC;
}

static inline const char *debugstr_us( const UNICODE_STRING * HOSTPTR us )
{
    if (!us) return "<null>";
    return debugstr_wn( us->Buffer, us->Length / sizeof(WCHAR) );
}

/* convert from straight ASCII to Unicode without depending on the current codepage */
static inline void ascii_to_unicode( WCHAR * HOSTPTR dst, const char * HOSTPTR src, size_t len )
{
    while (len--) *dst++ = (unsigned char)*src++;
}

static inline void *get_signal_stack(void)
{
    return (void *)(((ULONG_PTR)NtCurrentTeb() & ~signal_stack_mask) + teb_size);
}

static inline BOOL is_inside_signal_stack( void *ptr )
{
    return ((char *)ptr >= (char *)get_signal_stack() &&
            (char *)ptr < (char *)get_signal_stack() + signal_stack_size);
}

static inline void mutex_lock( pthread_mutex_t *mutex )
{
    if (!process_exiting) pthread_mutex_lock( mutex );
}

static inline void mutex_unlock( pthread_mutex_t *mutex )
{
    if (!process_exiting) pthread_mutex_unlock( mutex );
}

static inline async_data_t server_async( HANDLE handle, struct async_fileio *user, HANDLE event,
                                         PIO_APC_ROUTINE apc, void * apc_context, client_ptr_t iosb )
{
    async_data_t async;
    async.handle      = wine_server_obj_handle( handle );
    async.user        = wine_server_client_ptr( user );
    async.iosb        = iosb;
    async.event       = wine_server_obj_handle( event );
    async.apc         = wine_server_client_ptr( apc );
    async.apc_context = wine_server_client_ptr( apc_context );
    return async;
}

static inline NTSTATUS wait_async( HANDLE handle, BOOL alertable )
{
    return NtWaitForSingleObject( handle, alertable, NULL );
}

static inline BOOL in_wow64_call(void)
{
#ifdef _WIN64
    return !!NtCurrentTeb()->WowTebOffset;
#endif
    return FALSE;
}

static inline void set_async_iosb( client_ptr_t iosb, NTSTATUS status, ULONG_PTR info )
{
    if (!iosb) return;

    if (in_wow64_call())
    {
        struct iosb32
        {
            NTSTATUS Status;
            ULONG    Information;
        } *io = wine_server_get_ptr( iosb );
        io->Status = status;
        io->Information = info;
    }
    else
    {
        IO_STATUS_BLOCK *io = wine_server_get_ptr( iosb );
#ifdef NONAMELESSUNION
        io->u.Status = status;
#else
        io->Status = status;
#endif
        io->Information = info;
    }
}

static inline client_ptr_t iosb_client_ptr( IO_STATUS_BLOCK *io )
{
#ifdef NONAMELESSUNION
    if (io && in_wow64_call()) return wine_server_client_ptr( io->u.Pointer );
#else
    if (io && in_wow64_call()) return wine_server_client_ptr( io->Pointer );
#endif
    return wine_server_client_ptr( io );
}

#ifdef _WIN64
typedef TEB32 WOW_TEB;
typedef PEB32 WOW_PEB;
static inline TEB64 *NtCurrentTeb64(void) { return NULL; }
#else
typedef TEB64 WOW_TEB;
typedef PEB64 WOW_PEB;
static inline TEB64 *NtCurrentTeb64(void) { return (TEB64 *)NtCurrentTeb()->GdiBatchCount; }
#endif

struct xcontext
{
    CONTEXT c;
    CONTEXT_EX c_ex;
    ULONG64 host_compaction_mask;
};

#if defined(__i386__) || defined(__i386_on_x86_64__) || defined(__x86_64__)
extern BOOL xstate_compaction_enabled DECLSPEC_HIDDEN;

static inline XSTATE *xstate_from_context( const CONTEXT *context )
{
    CONTEXT_EX *xctx = (CONTEXT_EX *)(context + 1);

    if ((context->ContextFlags & CONTEXT_XSTATE) != CONTEXT_XSTATE)
        return NULL;

    return (XSTATE *)((char *)(context + 1) + xctx->XState.Offset);
}

static inline void context_init_xstate( CONTEXT *context, void *xstate_buffer )
{
    CONTEXT_EX *xctx;

    xctx = (CONTEXT_EX *)(context + 1);
    xctx->Legacy.Length = sizeof(CONTEXT);
    xctx->Legacy.Offset = -(LONG)sizeof(CONTEXT);

    xctx->XState.Length = sizeof(XSTATE);
    xctx->XState.Offset = (BYTE *)xstate_buffer - (BYTE *)xctx;

    xctx->All.Length = sizeof(CONTEXT) + xctx->XState.Offset + xctx->XState.Length;
    xctx->All.Offset = -(LONG)sizeof(CONTEXT);
    context->ContextFlags |= 0x40;
}

static inline void xstate_to_server( context_t *to, const XSTATE *xs )
{
    if (!xs)
        return;

    to->flags |= SERVER_CTX_YMM_REGISTERS;
    if (xs->Mask & 4)
        memcpy(&to->ymm.regs.ymm_high, &xs->YmmContext, sizeof(xs->YmmContext));
    else
        memset(&to->ymm.regs.ymm_high, 0, sizeof(xs->YmmContext));
}

static inline void xstate_from_server_( XSTATE *xs, const context_t *from, BOOL compaction_enabled)
{
    if (!xs)
        return;

    xs->Mask = 0;
    xs->CompactionMask = compaction_enabled ? 0x8000000000000004 : 0;

    if (from->flags & SERVER_CTX_YMM_REGISTERS)
    {
        unsigned long *src = (unsigned long *)&from->ymm.regs.ymm_high;
        unsigned int i;

        for (i = 0; i < sizeof(xs->YmmContext) / sizeof(unsigned long); ++i)
            if (src[i])
            {
                memcpy( &xs->YmmContext, &from->ymm.regs.ymm_high, sizeof(xs->YmmContext) );
                xs->Mask = 4;
                break;
            }
    }
}
#define xstate_from_server( xs, from ) xstate_from_server_( xs, from, user_shared_data->XState.CompactionEnabled )

#else
static inline XSTATE *xstate_from_context( const CONTEXT *context )
{
    return NULL;
}
static inline void context_init_xstate( CONTEXT *context, void *xstate_buffer )
{
}
#endif

extern WOW_PEB *wow_peb DECLSPEC_HIDDEN;

static inline WOW_TEB *get_wow_teb( TEB *teb )
{
    return teb->WowTebOffset ? (WOW_TEB *)((char *)teb + teb->WowTebOffset) : NULL;
}

enum loadorder
{
    LO_INVALID,
    LO_DISABLED,
    LO_NATIVE,
    LO_BUILTIN,
    LO_NATIVE_BUILTIN,  /* native then builtin */
    LO_BUILTIN_NATIVE,  /* builtin then native */
    LO_DEFAULT          /* nothing specified, use default strategy */
};

extern void set_load_order_app_name( const WCHAR * HOSTPTR app_name ) DECLSPEC_HIDDEN;
extern enum loadorder get_load_order( const UNICODE_STRING *nt_name ) DECLSPEC_HIDDEN;

static inline void init_unicode_string( UNICODE_STRING* str, const WCHAR * data )
{
    str->Length = wcslen(data) * sizeof(WCHAR);
    str->MaximumLength = str->Length + sizeof(WCHAR);
    str->Buffer = (WCHAR *)data;
}

#ifdef __i386_on_x86_64__
#define RtlAllocateHeap(heap,flags,size)    (pRtlAllocateHeap(heap,flags,size))
#define RtlFreeHeap(heap,flags,ptr)         (pRtlFreeHeap(heap,flags,ptr))
#else
#define RtlAllocateHeap(heap,flags,size)    (((flags) & HEAP_ZERO_MEMORY) ? calloc(1,size) : malloc(size))
#define RtlFreeHeap(heap,flags,ptr)         (free(ptr))
#endif

#endif /* __NTDLL_UNIX_PRIVATE_H */
