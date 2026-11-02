# 0.23.10 — remove repeated full finisher searches during combat

Existing 09:18/09:20 captures show the VR finisher search averaging about1.22ms
per application update, with individual maxima8.54–10ms. No grounded target
was selected in their sampled diagnostic messages. The implementation made
two VirtualQuery-based readability checks for every native interaction in the
ROI before checking distance. A positive target additionally disabled the80ms
scan cadence, and holding the execution chord forced a full scan every frame.
These are separate costs from native AI and the large PVS stalls.

Global controller focus hooks also receive NPC targeting queries. Their player
ownership check previously performed a VirtualQuery and player-property lookup
on every call, even for NPCs. This now resolves the player's focus component
once per application frame and compares only numeric identities afterwards.
Player changes (including transformations) refresh immediately; world-stop
clears the cache. Component replacement is observed by the next application
update. NPC calls still execute their original focus routines without VR aim
overrides; no NPC targeting or AI is skipped. No cached pointer is dereferenced.

Full scans now filter distance before NPC-property queries and read the current
game-owned ROI through a narrow SEH boundary. An inaccessible entry is rejected
at the actual access, without thousands of kernel memory queries for a valid
native list. The player, range, grounded/alive status, species, ray/bounds
selection, occlusion and native execution routine remain checked. No native
AI, physics, damage or world-update frequency is reduced.

A successful selection records only the ROI index and numeric identities,
keyed to player, weapon and tracking session. It does not own an entity or retain
a dereferenced borrowed pointer. Every reused selection is resolved again from
the live native ROI and fully revalidated for eligibility, aim and line of sight.
Removal, reordering, identity replacement, death, changed aim or blocked sight
reject the hint and request an immediate full search. The complete ranking is
also refreshed every80ms. A new execution-chord press forces a fresh complete
ranking immediately; holding it continues validating the selected target
without rescanning all objects every frame. Menus/equipment/session/world-stop
invalidate the hint. No native SDK object is retained across frames or destroyed
during DLL detach; existing shutdown ownership is preserved.

The earlier896.8077ms PVS example in session11592/frame6092 occurred when the
camera moved from approximately(94436,5317,-12054) to(95769,2379,33559) SDK
units, after selecting It_Teleport_King. That is roughly457m with the configured
100units/metre. The PUP forced counter rose2->3. This confirms long-distance
zone loading in that example; it is not evidence that the same operation is
responsible for sustained low FPS in the initial Ardea battle. Mandatory
synchronous loading and native culling are therefore preserved. The181ms
occlusion example remains a separate measured scope, not assumed to be the
same cause as persistent crowded-scene performance.

0.23.9's native AI/animation/physics/streaming instrumentation remains available
for the next normal headset session. No live game was launched while the user
was away via Remote Desktop. Offline tests and the guarded-read microbenchmark
validate the changed search behavior; they do not establish an Ardea FPS gain.
