/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _GRIDREFMANAGER
#define _GRIDREFMANAGER

#include "RefMgr.h"
#include <algorithm>
#include <cstdint>

template<class OBJECT>
class GridReference;

template<class OBJECT>
class GridRefMgr : public RefMgr<GridRefMgr<OBJECT>, OBJECT>
{
public:
    typedef LinkedListHead::Iterator< GridReference<OBJECT> > iterator;

    GridRefMgr() = default;
    GridRefMgr(GridRefMgr const&) = delete;
    GridRefMgr& operator=(GridRefMgr const&) = delete;
    GridRefMgr(GridRefMgr&&) = delete;
    GridRefMgr& operator=(GridRefMgr&&) = delete;
    ~GridRefMgr() override
    {
        // Invalidate while derived cursor state still exists; the base destructor then sees an empty list.
        this->clearReferences();
    }

    struct BoundedVisitResult
    {
        uint32 examined = 0;
        bool complete = false;
        std::uint64_t revision = 0;
    };

    // Opt-in owner-thread traversal. No cursor escapes the container. Ordinary iteration is unchanged.
    // The callback may unlink/delete nodes, but may not destroy this manager or bypass Reference lifecycle.
    template<class Visitor>
    BoundedVisitResult VisitBounded(uint32 limit, Visitor&& visit)
    {
        BoundedVisitResult result;
        result.revision = _boundedRevision;
        if (_boundedVisiting)
            return result;
        struct VisitingGuard
        {
            bool& flag;
            explicit VisitingGuard(bool& value) : flag(value) { flag = true; }
            ~VisitingGuard() { flag = false; }
        } guard(_boundedVisiting);
        uint32 const population = this->getSize();
        uint32 const count = std::min(limit, population);
        while (result.examined < count)
        {
            if (!_boundedNext)
                _boundedNext = getFirst();
            if (!_boundedNext)
                break;
            auto* current = _boundedNext;
            _boundedNext = current->next();
            ++result.examined;
            visit(current->GetSource()); // Do not touch current after the callback.
        }
        result.complete = result.examined == population && result.revision == _boundedRevision;
        result.revision = _boundedRevision;
        return result;
    }

    GridReference<OBJECT>* getFirst() { return (GridReference<OBJECT>*)RefMgr<GridRefMgr<OBJECT>, OBJECT>::getFirst(); }
    GridReference<OBJECT>* getLast() { return (GridReference<OBJECT>*)RefMgr<GridRefMgr<OBJECT>, OBJECT>::getLast(); }

    iterator begin() { return iterator(getFirst()); }
    iterator end() { return iterator(nullptr); }
    iterator rbegin() { return iterator(getLast()); }
    iterator rend() { return iterator(nullptr); }

private:
    friend class GridReference<OBJECT>;
    void BoundedLinkAdded() { ++_boundedRevision; }
    void BoundedBeforeUnlink(GridReference<OBJECT>* reference)
    {
        if (_boundedNext == reference)
            _boundedNext = reference->next();
        ++_boundedRevision;
    }

    GridReference<OBJECT>* _boundedNext = nullptr;
    std::uint64_t _boundedRevision = 0;
    bool _boundedVisiting = false;
};
#endif
