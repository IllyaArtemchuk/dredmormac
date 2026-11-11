#include "dredmor/world.hpp"
#include <stdexcept>

namespace dredmor {
World World::read(std::istream& input) {
    World world;
    std::string row;
    int spawns = 0;
    while (std::getline(input, row)) {
        if (!row.empty() && row.back() == '\r') row.pop_back();
        if (row.empty() || row.size() > 512 || world.rows_.size() >= 512)
            throw std::runtime_error("room rows must contain 1..512 cells; at most 512 rows");
        if (!world.rows_.empty() && row.size() != world.rows_[0].size())
            throw std::runtime_error("room must be rectangular");
        for (std::size_t x = 0; x < row.size(); ++x) {
            if (row[x] == '@') {
                world.player_ = {static_cast<int>(x), static_cast<int>(world.rows_.size())};
                ++spawns;
                row[x] = '.';
            } else if (row[x] != '#' && row[x] != '.') {
                throw std::runtime_error("unsupported harness tile; use # . @");
            }
        }
        world.rows_.push_back(row);
    }
    if (input.bad()) throw std::runtime_error("room input read failed");
    if (world.rows_.empty() || spawns != 1)
        throw std::runtime_error("room requires exactly one @ spawn");
    return world;
}
bool World::passable(Position p) const {
    return p.x >= 0 && p.y >= 0 && p.x < width() && p.y < height() && tile(p.x, p.y) == '.';
}
World World::restore(std::istream& input,std::uint64_t turn) {
    auto world=read(input);world.turn_=turn;return world;
}
void World::place_player(Position p,std::uint64_t turn) {
    if (!passable(p)) throw std::invalid_argument("player placement must be on passable ground");
    player_=p;turn_=turn;
}
std::string World::room_text() const {
    auto rows=rows_;rows.at(player_.y).at(player_.x)='@';
    std::string output;
    for (const auto& row:rows) output+=row+'\n';
    return output;
}
Outcome World::step(Action action) {
    Position next = player_;
    switch (action) {
    case Action::north: --next.y; break;
    case Action::south: ++next.y; break;
    case Action::west: --next.x; break;
    case Action::east: ++next.x; break;
    case Action::wait: ++turn_; return Outcome::waited;
    default: throw std::invalid_argument("invalid action");
    }
    if (!passable(next)) return Outcome::blocked;
    player_ = next;
    ++turn_;
    return Outcome::moved;
}
std::string World::snapshot() const {
    return "{\"turn\":" + std::to_string(turn_) + ",\"x\":" + std::to_string(player_.x)
        + ",\"y\":" + std::to_string(player_.y) + "}";
}
Action parse_action(char c) {
    switch (c) {
    case 'N': return Action::north;
    case 'S': return Action::south;
    case 'W': return Action::west;
    case 'E': return Action::east;
    case '.': return Action::wait;
    default: throw std::invalid_argument("actions must be N S W E .");
    }
}
}
