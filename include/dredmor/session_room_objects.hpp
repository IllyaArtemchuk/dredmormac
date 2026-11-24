#pragma once
#include "dredmor/room_runtime.hpp"
#include "dredmor/room_scopes.hpp"

namespace dredmor {
class Session;
// Actor/item subjects use their existing stable IDs. Other subjects are their
// one-based Session vector indices. They are never original object pointers.
enum class SessionRoomObjectKind {player,monster,item,custom,breakable,lever,trap,keeper};
struct SessionRoomObjectRef {
    SessionRoomObjectKind kind=SessionRoomObjectKind::custom;
    std::uint64_t subject=0;
};
struct SessionRoomObjectBinding {
    SessionRoomObjectRef reference;
    RoomObjectIdentity object;
};
// Value-owned registration order, independent of Session's typed storage. The
// population/lifetime owner explicitly registers and unregisters objects; this
// adapter does not infer original constructor order from typed collections.
class SessionRoomObjects {
public:
    explicit SessionRoomObjects(std::uint64_t level=1);
    const std::vector<SessionRoomObjectBinding>& bindings() const {return bindings_;}
    // Finalization captures the owning tile's room byte. Hordes bypass their
    // record finalizer and must pass false. Duplicate labels are reported just
    // as SetRoomID reports its diagnostic boundary; the new label is retained.
    RoomIdAssignment bind(const Session& session,SessionRoomObjectRef reference,
        const DungeonLayout& layout,bool finalize=true);
    RoomIdAssignment assign(std::uint64_t identity,std::string id,const Session& session,
        const DungeonLayout& layout,bool debug=false);
    bool unregister(std::uint64_t identity);
    // Called when the typed owner erases an element. Static vector indices
    // shift; stable actor/item IDs do not. Remaining opaque IDs stay unchanged.
    void erase_subject(SessionRoomObjectRef reference);
    std::optional<ScriptObjectState> object(const Session& session,std::uint64_t identity) const;
    std::optional<ScriptObjectState> find(const Session& session,std::int32_t room,std::string_view id) const;
    std::optional<std::uint64_t> player() const;
    bool set_event(const Session& session,RoomEventQueue& events,SessionRoomObjectRef reference,std::string_view name) const;
    void save(std::ostream& output,const Session& session) const;
    static SessionRoomObjects load(const XmlNode& node,const Session& session);
private:
    std::uint64_t level_=1,next_=1;
    std::vector<SessionRoomObjectBinding> bindings_;
    std::vector<RoomObjectIdentity> registered(const Session& session) const;
};
}
