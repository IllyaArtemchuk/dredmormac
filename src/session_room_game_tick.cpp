#include "dredmor/session_room_game_tick.hpp"
#include <algorithm>
#include <stdexcept>

namespace dredmor {
SessionRoomScriptPass SessionRoomGameTickExecutor::run(Session& session,const SessionRoomScriptProfile& profile,
    SessionRoomObjects& objects,RoomRuntimeState& runtime,GameTickState& state,const SessionRoomGameTickHooks& hooks) {
    if (!hooks.perform || !hooks.ticks || !state.level || !objects.player())
        throw std::invalid_argument("scripted game tick requires a level, registered player and subsystem/clock hooks");
    auto candidate=session;auto bindings=objects;auto scripts=runtime;auto driver=state;
    std::optional<SessionRoomScriptQueue> queue;SessionRoomScriptPass result;
    std::size_t script_begin=0,script_count=0,teleport_index=0;
    auto refresh=[&](Session& owner,GameTickState& tick) {
        if (!tick.level || tick.level->width!=profile.layout.width() || tick.level->height!=profile.layout.height() ||
            tick.level->width!=owner.world_.width() || tick.level->height!=owner.world_.height())
            throw std::runtime_error("scripted game tick changed its prepared level");
        tick.level->objects.clear();for (const auto& entry:bindings.bindings()) tick.level->objects.push_back(entry.object.identity);
        tick.player.position=owner.player_.position;tick.player.dead=owner.player_.alive()?0:1;
        tick.spell_cooldowns.assign(owner.spells_?owner.spells_->size():0,0);
        for (const auto& [index,remaining]:owner.spell_cooldowns_) tick.spell_cooldowns.at(index)=remaining;
    };
    auto publish_cooldowns=[](Session& owner,const GameTickState& tick) {
        if (tick.spell_cooldowns.size()!=(owner.spells_?owner.spells_->size():0))
            throw std::runtime_error("scripted game tick lost its spell cooldown projection");
        owner.spell_cooldowns_.clear();
        for (std::size_t i=0;i<tick.spell_cooldowns.size();++i) if (tick.spell_cooldowns[i]) {
            if (!owner.spells_->at(i).casting.cooldown) throw std::runtime_error("cooldown belongs to a non-cooldown spell");
            owner.spell_cooldowns_[i]=tick.spell_cooldowns[i];
        }
    };
    auto finish=[&](Session& owner) {
        if (queue && queue->stage!=SessionRoomScriptStage::finished)
            result=SessionRoomScriptExecutor::finish(owner,bindings,*queue);
    };
    refresh(candidate,driver);SessionGameTickHooks bridge;
    bridge.perform=[&](Session& owner,GameTickState& tick,const TickRequest& request) {
        publish_cooldowns(owner,tick);bool script_owned=false;
        switch (request.kind) {
            case TickRequestKind::room_triggers:
                queue=SessionRoomScriptExecutor::evaluate(owner,profile,bindings,scripts,hooks.scripts);script_owned=true;break;
            case TickRequestKind::clear_destruction: {
                if (!queue) throw std::runtime_error("scripted tick cleared destruction before scripts");
                const auto destruction=queue->destruction;
                SessionRoomScriptExecutor::clear_destruction(owner,bindings,*queue);
                tick.teleports.erase(std::remove_if(tick.teleports.begin(),tick.teleports.end(),[&](const auto& move) {
                    return std::find(destruction.begin(),destruction.end(),move.object)!=destruction.end();
                }),tick.teleports.end());
                script_begin=tick.teleports.size();script_count=queue->moves.size();
                for (const auto& move:queue->moves) tick.teleports.push_back({move.subject,queue->level,move.point});
                script_owned=true;
                break;
            }
            case TickRequestKind::teleport:
                if (teleport_index>=script_begin && teleport_index-script_begin<script_count) {
                    const auto& move=queue->moves.at(queue->next_move);
                    if (request.object!=move.subject || request.level!=queue->level || request.position.x!=move.point.x || request.position.y!=move.point.y)
                        throw std::runtime_error("scripted tick teleport order changed");
                    SessionRoomScriptExecutor::place_next(owner,bindings,*queue);
                    script_owned=true;
                }
                ++teleport_index;break;
            case TickRequestKind::tutorial:finish(owner);break;
            default:break;
        }
        // Publish registry changes before the observer/remaining subsystem body.
        refresh(owner,tick);hooks.perform(owner,bindings,scripts,tick,request,script_owned);refresh(owner,tick);
    };
    bridge.ticks=[&](Session& owner,GameTickState& tick) {
        publish_cooldowns(owner,tick);finish(owner);refresh(owner,tick);
        const auto now=hooks.ticks(owner,tick);refresh(owner,tick);return now;
    };
    SessionGameTickExecutor::run(candidate,driver,bridge);
    if (!queue || queue->stage!=SessionRoomScriptStage::finished) throw std::runtime_error("scripted game tick did not finish its room phase");
    session=std::move(candidate);objects=std::move(bindings);runtime=std::move(scripts);state=std::move(driver);return result;
}
}
