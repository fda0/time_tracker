#if Def_Windows
typedef FILETIME File_Time;
typedef *LPTHREAD_START_ROUTINE New_Thread_Function;

#elif Def_Linux
typedef time_t File_Time;
typedef void *New_Thread_Function(void *);

#else
#error "platform unsupported"
#endif