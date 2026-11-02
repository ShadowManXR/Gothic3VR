#pragma once
#include "FramePerformance.h"
namespace diagnostic::performance {
enum class NativePhase : unsigned {SessionProcess,Focus,FrustumUpdate,FrustumExtract,FrustumOcclusionExtract,FrustumExecute,RendererRender,RangedScript,ProjectilePrepare,ProjectileShoot,FrustumRefresh,PvsProcess,PvsExecute,PvsWait,PupWork,PvsGrid,PvsCells,CellHierarchy,GeometryCacheIn,GeometryCacheOut,GeometryReadNodes,PoolQueueWait,PoolIdleWait,ModuleUpdate,EntityUpdate,CharacterAnimation,PhysicsSimulate,AnimationPhysics,AiRoutine,Navigation,SampledRenderOperation,SampledInstanceBatches,SampledMaterialBegin,SampledMaterialEnd,SampledEffectExecute,SampledTextureExecute,SampledVisualExecute,SampledDynamicOccludees,SampledDecals,SampledInstanceItem,SampledShadowMap,SampledLowPoly,SampledZPassBegin,SampledZPassEnd,SampledAnimatedZPass,SampledMeshZPass,SampledRenderOperationZ,SampledSolidBegin,SampledSolidEnd,SampledLowPolyRenderer,SampledNonLowPolyRenderer,Count};
inline constexpr unsigned NativePhaseCount=unsigned(NativePhase::Count);
inline constexpr char const* NativePhaseNames[]={"native_session_process_inclusive","controller_focus_query_inclusive","frustum_update_inclusive","frustum_extract_inclusive","frustum_occlusion_extract_inclusive","frustum_execute_inclusive","render_system_render_inclusive","native_ranged_shoot_script_inclusive","vr_ranged_projectile_prepare","native_projectile_shoot","frustum_refresh_inclusive","pvs_prefetch_process_inclusive","pvs_prefetch_execute_inclusive","pvs_prefetch_wait_inclusive","pup_optional_queue_work_inclusive","pvs_grid_rebuild_inclusive","pvs_valid_cells_inclusive","cell_static_hierarchy_inclusive","geometry_cache_in_inclusive","geometry_cache_out_inclusive","geometry_read_nodes_inclusive","thread_pool_queue_wait_inclusive","thread_pool_idle_wait_inclusive","engine_modules_update_inclusive","engine_entities_update_inclusive","character_animation_inclusive","physics_simulate_inclusive","animation_physics_inclusive","ai_script_routine_inclusive","navigation_update_inclusive","sampled_render_operation_inclusive","sampled_instance_batches_inclusive","sampled_material_begin_inclusive","sampled_material_end_inclusive","sampled_effect_execute_inclusive","sampled_texture_execute_inclusive","sampled_visual_execute_inclusive","sampled_dynamic_occludees_inclusive","sampled_decals_inclusive","sampled_instance_item_inclusive","sampled_shadow_map_inclusive","sampled_low_poly_inclusive","sampled_zpass_begin_inclusive","sampled_zpass_end_inclusive","sampled_animated_zpass_inclusive","sampled_mesh_zpass_inclusive","sampled_render_operation_z_inclusive","sampled_solid_begin_callbacks_inclusive","sampled_solid_end_callbacks_inclusive","sampled_pup_low_poly_renderer_inclusive","sampled_pup_non_low_poly_renderer_inclusive"};
static_assert(sizeof(NativePhaseNames)/sizeof(NativePhaseNames[0])==NativePhaseCount);
struct NativePhaseReport {
    int context=0;std::array<uint64_t,NativePhaseCount> calls{};
    std::array<double,NativePhaseCount> meanMs{},maxMs{};
    bool Json(char* out,std::size_t size)const{
        if(!out||!size)return false;std::size_t used=0;
        auto append=[&](char const* format,auto... args){int n=std::snprintf(out+used,size-used,format,args...);if(n<0||std::size_t(n)>=size-used)return false;used+=std::size_t(n);return true;};
        if(!append("{\"type\":\"vr_native_phase_performance\",\"context\":%d,\"calls\":{",context))return false;
        for(unsigned i=0;i<NativePhaseCount;++i)if(!append("%s\"%s\":%llu",i?",":"",NativePhaseNames[i],static_cast<unsigned long long>(calls[i])))return false;
        if(!append("},\"cpu_wall_mean_per_call_ms\":{"))return false;
        for(unsigned i=0;i<NativePhaseCount;++i)if(!append("%s\"%s\":%.4f",i?",":"",NativePhaseNames[i],meanMs[i]))return false;
        if(!append("},\"cpu_wall_max_per_call_ms\":{"))return false;
        for(unsigned i=0;i<NativePhaseCount;++i)if(!append("%s\"%s\":%.4f",i?",":"",NativePhaseNames[i],maxMs[i]))return false;
        return append("},\"note\":\"inclusive application-thread CPU scopes overlap; scene/render phases inside VR eyes; sampled_* only one of 32 world stereo frames, excluded during temporal capture; simulation also outside eyes; session Process is not the entire engine loop\"}");
    }
};
// Independent of the application OnProcess callback: native session/focus
// processing may occur before or after that callback in the engine loop.
class NativePhaseCollector {
    uint64_t frequency=0,start=0;int context=-1;
    std::array<uint64_t,NativePhaseCount> ticks{},maximum{},calls{};
public:
    explicit NativePhaseCollector(uint64_t hz=Frequency()):frequency(hz){}
    void Reset(){start=0;context=-1;ticks={};maximum={};calls={};}
    bool Add(NativePhase phase,uint64_t elapsed,uint64_t now,int next,NativePhaseReport& out){
        unsigned i=unsigned(phase);if(i>=NativePhaseCount||!frequency)return false;
        if(context!=next||now<start){Reset();context=next;start=now;}
        ticks[i]+=elapsed;maximum[i]=(std::max)(maximum[i],elapsed);++calls[i];
        if(now-start<2*frequency)return false;
        out={};out.context=context;out.calls=calls;
        for(unsigned n=0;n<NativePhaseCount;++n){out.meanMs[n]=calls[n]?double(ticks[n])*1000/double(frequency)/double(calls[n]):0;out.maxMs[n]=double(maximum[n])*1000/double(frequency);}
        start=now;ticks={};maximum={};calls={};return true;
    }
};
}
