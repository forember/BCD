#include "slices.hh"

Connections::Connections(const std::vector<slice> &left_slices,
        const std::vector<slice> &right_slices)
{
    for (const slice &a : left_slices)
    {
        left.emplace(a, std::vector<slice>());
    }
    for (const slice &b : right_slices)
    {
        right.emplace(b, std::vector<slice>());
    }
    for (const slice &a : left_slices)
    {
        for (const slice &b : right_slices)
        {
            if (a.first < b.second && b.first < a.second)
            {
                left.at(a).push_back(b);
                right.at(b).push_back(a);
            }
        }
    }
}

std::vector<BCDEvent> Connections::find_events()
{
    std::vector<BCDEvent> events;
    for (const auto &a : left)
    {
        if (a.second.size() == 0)
        {
            BCDEvent event;
            event.event_type = BCD_EVENT_END;
            event.left.push_back(a.first);
            events.push_back(event);
        }
        else if (a.second.size() > 1)
        {
            BCDEvent event;
            event.event_type = BCD_EVENT_SPLIT;
            event.left.push_back(a.first);
            event.right = a.second;
            events.push_back(event);
        }
    }
    for (const auto &b : right)
    {
        if (b.second.size() == 0)
        {
            BCDEvent event;
            event.event_type = BCD_EVENT_START;
            event.right.push_back(b.first);
            events.push_back(event);
        }
        else if (b.second.size() > 1)
        {
            BCDEvent event;
            event.event_type = BCD_EVENT_MERGE;
            event.left = b.second;
            event.right.push_back(b.first);
            events.push_back(event);
        }
    }
    return events;
}

std::vector<slice> find_slices(const std::vector<bool> &column, bool match)
{
    std::vector<slice> slices;
    slice *current_slice = nullptr;
    bool previous_px = !match;
    for (size_t i = 0; i <= column.size(); ++i)
    {
        bool current_px = (i == column.size()) ? !match : column.at(i);
        if (previous_px != match && current_px == match)
        {
            slices.emplace_back(i, 0);
        }
        else if (previous_px == match && current_px != match)
        {
            slices.back().second = i;
        }
        previous_px = current_px;
    }
    return slices;
}

std::vector<std::vector<BCDEvent>> find_all_events(
    const std::vector<std::vector<bool>> &columns, bool match)
{
    std::vector<std::vector<BCDEvent>> events;
    std::vector<std::vector<slice>> all_slices;
    for (const auto &column : columns)
    {
        all_slices.push_back(find_slices(column, match));
    }
    for (size_t i = 0; i < all_slices.size() - 1; ++i)
    {
        Connections connections(all_slices.at(i), all_slices.at(i + 1));
        events.push_back(connections.find_events());
    }
    return events;
}