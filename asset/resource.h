#pragma once

#include <taskflow/taskflow.hpp>

#include <entt/container/dense_map.hpp>
#include <entt/core/hashed_string.hpp>
#include <entt/core/utility.hpp>
#include <entt/entity/registry.hpp>
#include <entt/signal/delegate.hpp>
#include <entt/signal/sigh.hpp>

#include <chrono>
#include <cstddef>
#include <functional>
#include <future>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace roj {

    
    struct resource_id { entt::id_type value{}; };

    template<class T>
    struct resource_job { std::future<std::optional<T>> result; };

    template<class T, class In>
    struct resource_loader { entt::delegate<std::optional<T>(const In&)> load; };


    namespace resource_detail {

        using time_point = std::chrono::steady_clock::time_point;

        template<class T>  
        struct table { entt::dense_map<entt::id_type, entt::entity, std::identity> entities; };

        struct pending { std::size_t jobs{}; };                                      
        struct kinds { entt::sigh<void(entt::registry&, time_point)> commits; };     

        
        inline void pendingInc(entt::registry& reg, entt::entity) { ++reg.ctx().get<pending>().jobs; }
        inline void pendingDec(entt::registry& reg, entt::entity) { --reg.ctx().get<pending>().jobs; }
        
        template<class In>
        struct input {
            using key = In;
            using store = In;
            static const key& view(const store& s) { return s; }
        };
        
        template<class U>
        struct input<std::reference_wrapper<U>> {
            using key = std::remove_const_t<U>;
            using store = std::reference_wrapper<U>;
            static const key& view(const store& s) { return s.get(); }
        };

        
        template<class U>
        struct input<std::shared_ptr<U>> {
            using key = std::remove_const_t<U>;
            using store = std::shared_ptr<U>;
            static const key& view(const store& s) { return *s; }
        };

        
        template<class T, class Arg>
        void spawn(entt::registry& reg, entt::entity e, Arg&& in) {
            using io = input<std::decay_t<Arg>>;
            auto* slot = reg.ctx().find<resource_loader<T, typename io::key>>();
            auto* pool = reg.ctx().find<tf::Executor>();
            ENTT_ASSERT(slot && slot->load, "resourceLoader<T, Input>(reg).connect<...>() was never called for this input type");
            ENTT_ASSERT(pool, "no tf::Executor in the registry context");
            reg.emplace_or_replace<resource_job<T>>(e, resource_job<T>{ pool->async(
                [load = slot->load, s = typename io::store(std::forward<Arg>(in))] { return load(io::view(s)); }) });
        }

        template<class T>
        bool isReady(const resource_job<T>& job) {
            return job.result.wait_for(std::chrono::seconds::zero()) == std::future_status::ready;
        }

        template<class T>
        void finish(entt::registry& reg, entt::entity e) {  
            auto value = reg.get<resource_job<T>>(e).result.get();
            reg.remove<resource_job<T>>(e);
            if (value) reg.emplace_or_replace<T>(e, std::move(*value));  
        }

        template<class T>
        void commit(entt::registry& reg, time_point until) {
            std::vector<entt::entity> done;  
            for (auto [e, job] : reg.view<resource_job<T>>().each())
                if (isReady(job)) done.push_back(e);
            for (auto e : done) {
                auto* job = reg.try_get<resource_job<T>>(e);
                if (!job || !isReady(*job)) continue;                   
                finish<T>(reg, e);
                if (std::chrono::steady_clock::now() >= until) break;   
            }
        }

    } 
    
    template<class T, class In>
    entt::delegate<std::optional<T>(const In&)>& resourceType(entt::registry& reg) {
        auto& ctx = reg.ctx();
        auto* slot = ctx.find<resource_loader<T, In>>();

        if (ctx.contains<resource_detail::table<T>>() && slot) return slot->load;
        ctx.emplace<resource_detail::table<T>>();
        if (!ctx.contains<resource_detail::kinds>()) {
            ctx.emplace<resource_detail::kinds>();
            ctx.emplace<resource_detail::pending>();
        }
        entt::sink{ ctx.get<resource_detail::kinds>().commits }.template connect<&resource_detail::commit<T>>();
        reg.on_construct<resource_job<T>>().template connect<&resource_detail::pendingInc>();
        reg.on_destroy<resource_job<T>>().template connect<&resource_detail::pendingDec>();
        return (slot ? *slot : ctx.emplace<resource_loader<T, In>>()).load;
    }

    
    template<class T>
    entt::entity resourceFind(const entt::registry& reg, entt::id_type id) {
        if (const auto* lookup = reg.ctx().find<resource_detail::table<T>>())
            if (auto it = lookup->entities.find(id); it != lookup->entities.end() && reg.valid(it->second))
                return it->second;
        return entt::null;
    }
    
    
    template<class T, class Arg>
    entt::entity resourceLoad(entt::registry& reg, entt::id_type id, Arg&& in) {
        if (const auto known = resourceFind<T>(reg, id); known != entt::null) return known;
        const auto e = reg.create();
        reg.emplace<resource_id>(e, id);
        reg.ctx().get<resource_detail::table<T>>().entities.insert_or_assign(id, e);
        resource_detail::spawn<T>(reg, e, std::forward<Arg>(in));
        return e;
    }

    
    
    template<class T, class Arg>
    entt::entity resourceLoad(entt::registry& reg, Arg&& in) {
        const auto e = reg.create();
        resource_detail::spawn<T>(reg, e, std::forward<Arg>(in));
        return e;   
    }
    
    template<class T, class Arg>
    entt::entity resourceReload(entt::registry& reg, entt::entity e, Arg&& in) {
        ENTT_ASSERT(reg.valid(e), "resourceReload on an invalid entity");
        resource_detail::spawn<T>(reg, e, std::forward<Arg>(in));
        return e;
    }
    
    template<class T>
    void resourceUnload(entt::registry& reg, entt::entity e) {
        if (!reg.valid(e)) return;
        if (const auto* id = reg.try_get<resource_id>(e))  
            if (auto* lookup = reg.ctx().find<resource_detail::table<T>>())
                if (auto it = lookup->entities.find(id->value); it != lookup->entities.end() && it->second == e)
                    lookup->entities.erase(it);
        reg.destroy(e);
    }

    
    template<class T>
    bool resourceWait(entt::registry& reg, entt::entity e) {
        if (!reg.valid(e)) return false;
        if (auto* job = reg.try_get<resource_job<T>>(e)) { job->result.wait(); resource_detail::finish<T>(reg, e); }
        return reg.all_of<T>(e);
    }

    template<class T>
    bool resourceFailed(const entt::registry& reg, entt::entity e) {
        return reg.valid(e) && !reg.any_of<T, resource_job<T>>(e);
    }

    
    inline void resourcesUpdate(entt::registry& reg,
        std::chrono::steady_clock::time_point until = std::chrono::steady_clock::time_point::max()) {
        if (auto* all = reg.ctx().find<resource_detail::kinds>()) all->commits.publish(reg, until);
    }

    inline std::size_t resourcesPending(const entt::registry& reg) {
        const auto* count = reg.ctx().find<resource_detail::pending>();
        return count ? count->jobs : 0u;
    }
    
    inline void resourcesFlush(entt::registry& reg) {
        auto* pool = reg.ctx().find<tf::Executor>();
        for (;;) {
            pool->wait_for_all();                    // workers idle; only we can submit
            if (resourcesPending(reg) == 0) return;  // nothing to commit, nothing started
            resourcesUpdate(reg);                    // hooks may submit; go round again
        }
    }

} // namespace roj