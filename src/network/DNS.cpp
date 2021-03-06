#include "DNS.h"
#include "NetException.h"
#include "SocketAddress.h"
#include "base/Environment.h"
//#include "NumberFormatter.h"
#include "base/Thread/MutexRW.h"
#include <cstring>


#if defined(PIL_HAVE_LIBRESOLV)
#include <resolv.h>
#endif

namespace pi {


#if defined(PIL_HAVE_LIBRESOLV)
static pi::MutexRW resolverLock;
#endif


HostEntry DNS::hostByName(const std::string& hostname, unsigned
#ifdef PIL_HAVE_ADDRINFO
                          hintFlags
#endif
                         )
{
#if defined(PIL_HAVE_LIBRESOLV)
    pi::ReadMutex readLock(resolverLock);
#endif

#if defined(PIL_HAVE_ADDRINFO)
    struct addrinfo* pAI;
    struct addrinfo hints;
    std::memset(&hints, 0, sizeof(hints));
    hints.ai_flags = hintFlags;
    int rc = getaddrinfo(hostname.c_str(), NULL, &hints, &pAI);
    if (rc == 0)
    {
//        std::cerr<<"Found entry for "<<hostname<<"\n";
        HostEntry result(pAI);
        freeaddrinfo(pAI);
        return result;
    }
    else
    {
        aierror(rc, hostname);
    }
#elif defined(PIL_VXWORKS)
    int addr = hostGetByName(const_cast<char*>(hostname.c_str()));
    if (addr != ERROR)
    {
        return HostEntry(hostname, IPAddress(&addr, sizeof(addr)));
    }
#else
    struct hostent* he = gethostbyname(hostname.c_str());
    if (he)
    {
        return HostEntry(he);
    }
#endif
    error(lastError(), hostname); // will throw an appropriate exception
    throw NetException(); // to silence compiler
}


HostEntry DNS::hostByAddress(const IPAddress& address, unsigned
#ifdef PIL_HAVE_ADDRINFO
                             hintFlags
#endif
                            )
{
#if defined(PIL_HAVE_LIBRESOLV)
    pi::ScopedReadRWLock readLock(resolverLock);
#endif

#if defined(PIL_HAVE_ADDRINFO)
    SocketAddress sa(address, 0);
    static char fqname[1024];
    int rc = getnameinfo(sa.addr(), sa.length(), fqname, sizeof(fqname), NULL, 0, NI_NAMEREQD);
    if (rc == 0)
    {
        struct addrinfo* pAI;
        struct addrinfo hints;
        std::memset(&hints, 0, sizeof(hints));
        hints.ai_flags = hintFlags;
        rc = getaddrinfo(fqname, NULL, &hints, &pAI);
        if (rc == 0)
        {
            HostEntry result(pAI);
            freeaddrinfo(pAI);
            return result;
        }
        else
        {
            aierror(rc, address.toString());
        }
    }
    else
    {
        aierror(rc, address.toString());
    }
#elif defined(PIL_VXWORKS)
    char name[MAXHOSTNAMELEN + 1];
    if (hostGetByAddr(*reinterpret_cast<const int*>(address.addr()), name) == OK)
    {
        return HostEntry(std::string(name), address);
    }
#else
    struct hostent* he = gethostbyaddr(reinterpret_cast<const char*>(address.addr()), address.length(), address.af());
    if (he)
    {
        return HostEntry(he);
    }
#endif
    int err = lastError();
    error(err, address.toString()); // will throw an appropriate exception
    throw NetException(); // to silence compiler
}


HostEntry DNS::resolve(const std::string& address)
{
    IPAddress ip;
    if (IPAddress::tryParse(address, ip))
        return hostByAddress(ip);
    else
        return hostByName(address);
}


IPAddress DNS::resolveOne(const std::string& address)
{
    const HostEntry& entry = resolve(address);
    if (!entry.addresses().empty())
        return entry.addresses()[0];
    else
        throw NoAddressFoundException(address);
}


HostEntry DNS::thisHost()
{
    return hostByName(hostName());
}


void DNS::reload()
{
#if defined(PIL_HAVE_LIBRESOLV)
    pi::WriteMutex writeLock(resolverLock);
    res_init();
#endif
}


void DNS::flushCache()
{
}


std::string DNS::hostName()
{
    char buffer[256];
    int rc = gethostname(buffer, sizeof(buffer));
    if (rc == 0)
        return std::string(buffer);
    else
        throw NetException("Cannot get host name");
}


int DNS::lastError()
{
#if defined(_WIN32)
    return GetLastError();
#elif defined(PIL_VXWORKS)
    return errno;
#else
    return h_errno;
#endif
}


void DNS::error(int code, const std::string& arg)
{
    switch (code)
    {
    case PIL_ESYSNOTREADY:
        throw NetException("Net subsystem not ready");
    case PIL_ENOTINIT:
        throw NetException("Net subsystem not initialized");
    case PIL_HOST_NOT_FOUND:
        throw HostNotFoundException(arg);
    case PIL_TRY_AGAIN:
        throw DNSException("Temporary DNS error while resolving", arg);
    case PIL_NO_RECOVERY:
        throw DNSException("Non recoverable DNS error while resolving", arg);
    case PIL_NO_DATA:
        throw NoAddressFoundException(arg);
    default:
        throw IOException((code));
    }
}


void DNS::aierror(int code, const std::string& arg)
{
#if defined(PIL_HAVE_IPv6) || defined(PIL_HAVE_ADDRINFO)
    switch (code)
    {
    case EAI_AGAIN:
        throw DNSException("Temporary DNS error while resolving", arg);
    case EAI_FAIL:
        throw DNSException("Non recoverable DNS error while resolving", arg);
#if !defined(_WIN32) // EAI_NODATA and EAI_NONAME have the same value
#if defined(EAI_NODATA) // deprecated in favor of EAI_NONAME on FreeBSD
    case EAI_NODATA:
        throw NoAddressFoundException(arg);
#endif
#endif
    case EAI_NONAME:
        throw HostNotFoundException(arg);
#if defined(EAI_SYSTEM)
    case EAI_SYSTEM:
        error(lastError(), arg);
        break;
#endif
#if defined(_WIN32)
    case WSANO_DATA: // may happen on XP
        throw HostNotFoundException(arg);
#endif
    default:
        throw DNSException("EAI", (code));
    }
#endif // PIL_HAVE_IPv6 || defined(PIL_HAVE_ADDRINFO)
}


} // namespace pi
