#ifndef Net_SocketAddress_INCLUDED
#define Net_SocketAddress_INCLUDED


#include "Net.h"
#include "SocketAddressImpl.h"
#include "base/Types/AutoPtr.h"


namespace pi {


class IPAddress;


class PIL_API SocketAddress
    /// This class represents an internet (IP) endpoint/socket
    /// address. The address can belong either to the
    /// IPv4 or the IPv6 address family and consists of a
    /// host address and a port number.
{
public:
    SocketAddress();
        /// Creates a wildcard (all zero) IPv4 SocketAddress.

    SocketAddress(const IPAddress& hostAddress, pi::UInt16 portNumber);
        /// Creates a SocketAddress from an IP address and given port number.

    SocketAddress(pi::UInt16 port);
        /// Creates a SocketAddress with unspecified (wildcard) IP address
        /// and given port number.

    SocketAddress(const std::string& hostAddress, pi::UInt16 portNumber);
        /// Creates a SocketAddress from an IP address and given port number.
        ///
        /// The IP address must either be a domain name, or it must
        /// be in dotted decimal (IPv4) or hex string (IPv6) format.

    SocketAddress(const std::string& hostAddress, const std::string& portNumber);
        /// Creates a SocketAddress from an IP address and the
        /// service name or port number.
        ///
        /// The IP address must either be a domain name, or it must
        /// be in dotted decimal (IPv4) or hex string (IPv6) format.
        ///
        /// The given port must either be a decimal port number, or
        /// a service name.

    explicit SocketAddress(const std::string& hostAndPort);
        /// Creates a SocketAddress from an IP address or host name and the
        /// port number/service name. Host name/address and port number must
        /// be separated by a colon. In case of an IPv6 address,
        /// the address part must be enclosed in brackets.
        ///
        /// Examples:
        ///     192.168.1.10:80
        ///     [::ffff:192.168.1.120]:2040
        ///     www.appinf.com:8080

    SocketAddress(const SocketAddress& addr);
        /// Creates a SocketAddress by copying another one.

    SocketAddress(const struct sockaddr* addr, pil_socklen_t length);
        /// Creates a SocketAddress from a native socket address.

    ~SocketAddress();
        /// Destroys the SocketAddress.

    SocketAddress& operator = (const SocketAddress& socketAddress);
        /// Assigns another SocketAddress.

    IPAddress host() const;
        /// Returns the host IP address.

    pi::UInt16 port() const;
        /// Returns the port number.

    pil_socklen_t length() const;
        /// Returns the length of the internal native socket address.

    const struct sockaddr* addr() const;
        /// Returns a pointer to the internal native socket address.

    int af() const;
        /// Returns the address family (AF_INET or AF_INET6) of the address.

    std::string toString() const;
        /// Returns a string representation of the address.

    IPAddress::Family family() const;
        /// Returns the address family of the host's address.

    bool operator < (const SocketAddress& socketAddress) const;
    bool operator == (const SocketAddress& socketAddress) const;
    bool operator != (const SocketAddress& socketAddress) const;

    enum
    {
        MAX_ADDRESS_LENGTH =
#if defined(PIL_HAVE_IPv6)
            sizeof(struct sockaddr_in6)
#else
            sizeof(struct sockaddr_in)
#endif
            /// Maximum length in bytes of a socket address.
    };

protected:
    void init(const IPAddress& hostAddress, pi::UInt16 portNumber);
    void init(const std::string& hostAddress, pi::UInt16 portNumber);
    pi::UInt16 resolveService(const std::string& service);

private:
    typedef pi::SocketAddressImpl Impl;
#ifdef PIL_HAVE_ALIGNMENT
    typedef Impl* Ptr;
#else
    typedef pi::AutoPtr<Impl> Ptr;
#endif

    Ptr pImpl() const;

    void newIPv4();

    void newIPv4(const sockaddr_in*);

    void newIPv4(const IPAddress& hostAddress, pi::UInt16 portNumber);

    void newIPv6(const sockaddr_in6*);

    void newIPv6(const IPAddress& hostAddress, pi::UInt16 portNumber);

    void destruct();

#ifdef PIL_HAVE_ALIGNMENT
    char* storage();

    #ifdef PIL_ENABLE_CPP11
        static const unsigned sz = sizeof(pi::IPv6SocketAddressImpl);
        typedef std::aligned_storage<sz>::type AlignerType;
        union
        {
            char buffer[sz];
        private:
            AlignerType aligner;
        }
    #else // !PIL_ENABLE_CPP11
        AlignedCharArrayUnion <pi::IPv6SocketAddressImpl>
    #endif // PIL_ENABLE_CPP11
        _memory;
#else // !PIL_HAVE_ALIGNMENT
    Ptr _pImpl;
#endif // PIL_HAVE_ALIGNMENT
};


//
// inlines
//


inline void SocketAddress::destruct()
{
#ifdef PIL_HAVE_ALIGNMENT
    pImpl()->~SocketAddressImpl();
#endif
}


inline SocketAddress::Ptr SocketAddress::pImpl() const
{
#ifdef PIL_HAVE_ALIGNMENT
    return reinterpret_cast<Ptr>(const_cast<char *>(_memory.buffer));
#else
    if (_pImpl) return _pImpl;
    throw pi::NullPointerException("Pointer to SocketAddress implementation is NULL.");
#endif
}


inline void SocketAddress::newIPv4()
{
#ifdef PIL_HAVE_ALIGNMENT
    new (storage()) pi::IPv4SocketAddressImpl;
#else
    _pImpl = new pi::IPv4SocketAddressImpl;
#endif
}


inline void SocketAddress::newIPv4(const sockaddr_in* sockAddr)
{
#ifdef PIL_HAVE_ALIGNMENT
    new (storage()) pi::IPv4SocketAddressImpl(sockAddr);
#else
    _pImpl = new pi::IPv4SocketAddressImpl(sockAddr);
#endif
}


inline void SocketAddress::newIPv4(const IPAddress& hostAddress, pi::UInt16 portNumber)
{
#ifdef PIL_HAVE_ALIGNMENT
    new (storage()) pi::IPv4SocketAddressImpl(hostAddress.addr(), htons(portNumber));
#else
    _pImpl = new pi::IPv4SocketAddressImpl(hostAddress.addr(), htons(portNumber));
#endif
}



inline void SocketAddress::newIPv6(const sockaddr_in6* sockAddr)
{
#ifdef PIL_HAVE_ALIGNMENT
    new (storage()) pi::IPv6SocketAddressImpl(sockAddr);
#else
    _pImpl = new pi::IPv6SocketAddressImpl(sockAddr);
#endif
}


inline void SocketAddress::newIPv6(const IPAddress& hostAddress, pi::UInt16 portNumber)
{
#ifdef PIL_HAVE_ALIGNMENT
    new (storage()) pi::IPv6SocketAddressImpl(hostAddress.addr(), htons(portNumber), hostAddress.scope());
#else
    _pImpl = new pi::IPv6SocketAddressImpl(hostAddress.addr(), htons(portNumber), hostAddress.scope());
#endif
}


inline IPAddress::Family SocketAddress::family() const
{
    return host().family();
}


#ifdef PIL_HAVE_ALIGNMENT
inline char* SocketAddress::storage()
{
    return _memory.buffer;
}
#endif


inline 	bool SocketAddress::operator == (const SocketAddress& socketAddress) const
{
    return host() == socketAddress.host() && port() == socketAddress.port();
}


inline bool SocketAddress::operator != (const SocketAddress& socketAddress) const
{
    return host() != socketAddress.host() || port() != socketAddress.port();
}


} // namespace pi


#endif // Net_SocketAddress_INCLUDED
