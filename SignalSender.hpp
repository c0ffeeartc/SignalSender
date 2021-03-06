#pragma once
#include <functional>
#include <vector>
#include <algorithm>


namespace sig
{


//
// http://codereview.stackexchange.com/questions/48594/unique-type-id-no-rtti
//
using typeid_t = void const*;

template <typename T>
static typeid_t
    type_id() noexcept
{
    static char const type_id{};

    return &type_id;
}


//
// SignalSender<Event> singleton
//      holds vector<std::function, objId>
//          connect() push_backs std::function
//          signal() iterates over std::function(const Event&)
//          disconnect() erases all matches to <classId: objId>
//          objId requires
//              Constructor(){static s_id=1; _id = s_id++;} // objId must be >0
//              or similar for each Subscriber
//              See for pitfalls:
//                  http://stackoverflow.com/questions/14585385/best-practice-how-to-get-a-unique-identifier-for-the-object
//
//
// May be used
//      through singleton sig::connect, sig::disconnect, sig::signal template functions
//      or through manually constructed SignalSender objects
//

template <typename Event>
class SignalSender
{
public:
    static SignalSender<Event>&
        getInstance(size_t reserveAmt = 1024)
    {
        static SignalSender<Event> s_instance(reserveAmt);
        return s_instance;
    }

    typedef
        std::pair< const void* // type_id<Subscriber>()
                 , size_t>     // objId
            ObjId;

    typedef
        std::function<void(const Event&) >
            Connection;

    typedef
        std::pair < ObjId ,Connection >
            ConnectionPair;

    typedef
        std::vector<ConnectionPair>
            Connections;


        SignalSender
            (size_t reserveAmt)
                : _reserveAmt(reserveAmt)
                , _reserveTimes(1)
    {
        connections.reserve(_reserveAmt);
    }


    template<typename Subscriber>
    void
        connect
            (Connection&& connection, size_t objId)
    {
        preReserve();
        connections.push_back(std::move( ConnectionPair
                    ( std::move(ObjId(type_id<Subscriber>(),objId))
                    , std::move(connection))));
    }


    void
        signal
            (const Event& event)
    {
        size_t sz = connections.size();
        for (size_t i=0; i< sz; ++i)
        {
            connections[i].second(event);
        }
    }


    template<typename Subscriber>
    void
        disconnect
            (size_t objId)
    {
        ObjId removeId(type_id<Subscriber>(), objId);

        auto endIt = connections.end();
        auto eraseBegin = std::remove_if( connections.begin(), endIt
                , [&removeId](const ConnectionPair&c){ return c.first == removeId; });
        connections.erase(eraseBegin, endIt);
    }


private:
    void preReserve()
    {
        size_t curReserve = _reserveAmt*_reserveTimes;
        if(connections.size() >= curReserve)
        {
            connections.reserve(_reserveAmt*(++_reserveTimes)); // algebraic progression
        }
    }


private:
     Connections connections;
     size_t _reserveAmt;
     size_t _reserveTimes;
};


//
// global FUNC
//      use void/etc for SUBSCRIBER type
// for objects use lambda with capture
// do not use OBJ_ID == 0, it may become special value in future
//
template<typename EVENT, typename SUBSCRIBER>
void connect(typename sig::SignalSender<EVENT>::Connection && func,size_t objId){
    sig::SignalSender<EVENT>::getInstance().template connect<SUBSCRIBER>
        (std::move(func), objId);
}


//
// Must be called if previously called CONNECT
//
template<typename EVENT, typename SUBSCRIBER>
void disconnect(size_t objId){
    sig::SignalSender<EVENT>::getInstance().template disconnect<SUBSCRIBER>(objId);
}

template<typename EVENT>
void signal(EVENT &e){
    sig::SignalSender<EVENT>::getInstance().signal(e);
}

} // namespace sig
