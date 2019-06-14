
#ifndef __Win32__
#include <sys/types.h>
#include <dirent.h>
#endif
#include <errno.h>

#ifndef __Win32__
#include <unistd.h>
#include <grp.h>
#include <pwd.h>
#endif

#include "OSMemory.h"
#include "OSArrayObjectDeleter.h"
#include "SocketUtils.h"
#include "TCPListenerSocket.h"
#include "Task.h"

// CLASS DEFINITIONS

class ADDR_TCPListenerSocket : public TCPListenerSocket
{
    public:
        ADDR_TCPListenerSocket() {}
        virtual ~ADDR_TCPListenerSocket() {}
        
        //sole job of this object is to implement this function
        virtual Task*   GetSessionTask(TCPSocket** outSocket);
        
        //check whether the Listener should be idling
        static Bool16 OverMaxConnections();
    private:
        static UInt32      sActiveConnections;
        int                 TaskType;
};