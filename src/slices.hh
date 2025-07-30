#ifndef SLICES_HH
#define SLICES_HH

#include <map>
#include <stdlib.h>
#include <vector>

typedef std::pair<size_t, size_t> slice;

enum BCDEventType
{
    BCD_EVENT_START,
    BCD_EVENT_END,
    BCD_EVENT_MERGE,
    BCD_EVENT_SPLIT
};

struct BCDEvent
{
    BCDEventType event_type;
    std::vector<slice> left;
    std::vector<slice> right;
};

class Connections
{
private:
    std::map<slice, std::vector<slice>> left;
    std::map<slice, std::vector<slice>> right;

public:
    Connections(const std::vector<slice> &left_slices,
        const std::vector<slice> &right_slices);
    std::vector<BCDEvent> find_events();
};

std::vector<slice> find_slices(const std::vector<bool> &column, bool match);
std::vector<std::vector<BCDEvent>> find_all_events(
    const std::vector<std::vector<bool>> &columns, bool match);

#endif