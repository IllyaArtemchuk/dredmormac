#pragma once
#include "dredmor/session.hpp"

namespace dredmor {
enum class FloorTravel {travelled,no_stair,missing_floor,missing_arrival,dead};
// Prepared-floor registry. It preserves floor entities and the current global
// character/RNG across visits. Off-floor regeneration and lazy level generation
// are separate from this cache integration.
class FloorRegistry {
public:
    const std::vector<SessionFloor>& cached() const {return cached_;}
    void add_floor(Session& session,SessionFloor floor);
    FloorTravel travel(Session& session,bool ascending);
    FloorTravel step(Session& session,Action action);
    void save(const Session& session,std::ostream& output) const;
    void for_each_floor(const Session& session,const std::function<void(const Session&)>& visitor) const;
    static std::pair<Session,FloorRegistry> load(std::istream& input,
        std::shared_ptr<const std::vector<ItemDefinition>> definitions={},
        std::shared_ptr<const std::vector<AbilityDefinition>> abilities={},
        std::shared_ptr<const std::vector<SpellDefinition>> spells={});
private:
    std::vector<SessionFloor> cached_;
};
}
