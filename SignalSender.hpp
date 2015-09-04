#pragma once
#include "type_id.hpp"
#include <functional>
#include <vector>
#include <stack>
#include <algorithm>


namespace sig
{

//
// SignalSender<Event> singleton
//      holds vector<std::function, objId>
//          connect() push_backs std::function
//          signal() iterates over std::function(const Event&)
//          remove() erases all matches to <classId: objId>
//          objId requires
//              Constructor(){static s_id=1; _id = s_id++;}
//              or similar for each Subscriber
//              See for pitfalls:
//                  http://stackoverflow.com/questions/14585385/best-practice-how-to-get-a-unique-identifier-for-the-object
//
// Exposes CONNECT, DISCONNECT, SIGNAL macros for convenience
//      if previously called CONNECT
//          DISCONNECT *must* be called
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

    SignalSender(size_t reserveAmt)
    {
        connections.reserve(reserveAmt);
    }

    template<typename Subscriber>
    void
        connect
            (Connection&& connection, size_t objId)
    {
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
        remove
            (size_t objId)
    {
        ObjId removeId(type_id<Subscriber>(), objId);

        auto it    = connections.begin();
        auto endIt = connections.end();

        // collect
        //
        std::stack<typename Connections::iterator> st;
        for(;;)
        {
            it = find_if (it, endIt, [&removeId](const ConnectionPair&c){ return c.first == removeId;});
            if(it == endIt) break;
            st.push(it);
            it++;
        }

        // remove
        //
        while(!st.empty())
        {
            connections.erase(st.top());
            st.pop();
        }
    }

private:
     Connections connections;
};
} // namespace sig


//
// global - use void/etc for SUBSCRIBER type
// for objects use lambda with capture
//
#define CONNECT(EVENT, FUNC, SUBSCRIBER, OBJ_ID)\
    sig::SignalSender<EVENT>::getInstance().connect<SUBSCRIBER>\
        (sig::SignalSender<EVENT>::Connection(FUNC), OBJ_ID);


//
// Must be called if previously called CONNECT
//
#define DISCONNECT(EVENT, SUBSCRIBER, OBJ_ID)\
    sig::SignalSender<EVENT>::getInstance().remove<SUBSCRIBER>(OBJ_ID);


#define SIGNAL(EVENT, EVENT_OBJ)\
    sig::SignalSender<EVENT>::getInstance().signal(EVENT_OBJ)



