ROADMAP (detailed). Elo figures are rough signposts only and vary by
rating list and hardware. Every phase ends only when its exit criterion is
met and shown with real output.

PHASE 0: SETUP AND TOOLING [DONE]
Goal: a reproducible build and a way to measure strength.
Tasks: compiler, CMake, Git, profiler, sanitizers, fastchess, Stockfish
binary, a balanced opening book (paired openings), a project folder.
Exit: one command builds Release and Debug, and fastchess can run a match
between two existing engines.

PHASE 1: THEORY STUDY
Goal: understand each technique well enough to implement it myself.
Topics, each with a worked example, common mistakes, and 3 quiz questions:
bitboards and square mapping; magic/PEXT bitboards; legal movegen (pins,
checks, en passant, castling); perft and divide; negamax and alpha-beta;
iterative deepening and quiescence; Zobrist hashing and transposition
tables; move ordering (hash move, MVV-LVA, SEE, killers, history);
null-move pruning, LMR, futility pruning; evaluation basics (material,
piece-square tables, tapered eval); NNUE overview.
Exit: I can explain how negamax with alpha-beta, a transposition table,
and quiescence search fit together, in my own words, without notes.
Pitfall: reading without implementing. Study each topic right before
building it.

PHASE 2: STUDY EXISTING ENGINES
Goal: learn how real engines structure code and which ideas matter.
Order: TSCP or Vice (simple), then Weiss, Ethereal, Berserk (clean and
strong), and Stockfish last, one feature at a time. For Rust, Viridithas
and Black Marlin are options.
Method: study by feature, not file by file. Pick one technique, find how
two or three engines implement it, write notes comparing them, then
implement my own version and measure it. Read Stockfish commit history and
fishtest results to see which ideas gained and which failed.
Licenses: Stockfish is GPL. Learn ideas, write my own code.
Exit: a notes file covering each core technique, comparing at least two
engines per technique.

PHASE 3: BITBOARDS AND LEGAL MOVE GENERATION [IN PROGRESS]
Goal: a correct legal move generator proven by perft. Correctness is
non-negotiable because every later phase depends on it.
Steps 1-8 as listed in PHASE 3 DETAILS.
Pitfalls: edge squares in sliding masks, shifting by 64, en passant
discovered check along a rank, castling rights not updated when a rook is
captured, promotion captures, forgetting to clear the EP square.
Exit: all six perft positions match the published counts at every listed
depth with zero discrepancies, under ASan/UBSan in Debug. Then
measure NPS (a decent target on this CPU is tens of millions of nodes per
second in perft, single core, but treat that as a rough guide).

PHASE 4: BASIC PLAYABLE ENGINE (~1800-2200 Elo, rough)
Goal: an engine that plays complete legal games.
Tasks:
 a. Static evaluation: material plus piece-square tables, scored from the
    side-to-move perspective (this is what negamax needs).
 b. Negamax with alpha-beta and iterative deepening. Return mate scores
    with distance so the engine prefers faster mates.
 c. Quiescence search: captures (and later promotions/check evasions) to
    avoid the horizon effect. Use stand-pat.
 d. Move ordering: captures first by MVV-LVA.
 e. Transposition table: Zobrist key, depth, score, bound type (exact,
    lower, upper), best move, and a replacement scheme. Adjust mate scores
    when storing and probing.
 f. Draw detection: threefold repetition (using the key history from the
    Position), 50-move rule at halfmove clock 100 (checkmate on that move
    still wins), insufficient material.
Tests: mate-in-1/2/3 puzzle suites, a search that finds the same best move
with and without the TT in non-hash-sensitive positions, no illegal
moves across hundreds of self-play games.
Pitfalls: mixing up score perspective, storing mate scores raw in the TT,
bound type mistakes, a TT that returns wrong scores because of
repetition or 50-move state, and quiescence search explosions.
Exit: plays full games without crashes or illegal moves, beats a random
mover and a depth-limited naive engine, and finds simple mates.

PHASE 5: UCI AND THE TESTING HARNESS
Goal: reliable measurement. Do this BEFORE tuning anything.
Tasks:
 a. UCI: uci, isready, ucinewgame, position (startpos/fen + moves), go
    (depth, nodes, movetime, wtime, btime, winc, binc, infinite), stop,
    quit, setoption (Hash, Threads), info lines (depth, score, nodes, nps,
    pv).
 b. A "bench" command: search a fixed set of positions to a fixed depth
    and print total nodes and NPS. The node count is a deterministic
    signature; if it changes unexpectedly, behavior changed.
 c. SPRT testing with fastchess: new build vs baseline build, balanced
    paired openings, fixed time control, bounds such as [0, 3] or [0, 5]
    Elo for ordinary changes, a script that builds both versions, runs the
    test, and stores the output.
 d. A changelog file recording every idea, its SPRT result, and the
    Elo gain. Failed ideas are valuable.
Hardware notes: laptop plugged in, fastchess concurrency 4 to 5 (not all
12 threads), no other heavy programs, consistent power plan.
Pitfalls: testing at a time control too short to matter, too few games,
stopping a test early on a lucky streak, comparing builds with different
settings.
Exit: the engine runs correctly in a GUI and under fastchess, and one
command runs an SPRT test end to end.

PHASE 6: SEARCH ENHANCEMENTS (~2600-3000 Elo, rough)
Goal: most of the Elo gain. One technique at a time, each SPRT-tested; keep
only what gains, and re-tune related constants after interactions.
Techniques, in a sensible order:
 1. Principal variation search (zero-window for non-PV moves, re-search on
    fail-high).
 2. Better move ordering: TT move, good captures by SEE, killers,
    history heuristic, countermoves, continuation history.
 3. Null-move pruning (not in check, not in zugzwang-prone endgames, with a
    verification or a pawn-only-material guard).
 4. Late move reductions with a tuned reduction table.
 5. Reverse futility pruning and futility pruning.
 6. Late move pruning.
 7. Check extensions, then singular extensions (test carefully).
 8. Aspiration windows.
 9. Internal iterative reduction (no TT move at high depth).
10. SEE pruning in search and quiescence.
11. TT improvements: buckets, aging, depth-preferred replacement, storing
    static eval.
Pitfalls: pruning in check, pruning mate scores, reductions applied to
the TT move, interactions between techniques (a gain at one stage can
become a loss later), and trusting a result from too few games.
Exit: cumulative SPRT-verified gains, and the engine beats the small
open-source engines from Phase 2 at equal time.

PHASE 7: STRONGER HANDCRAFTED EVALUATION AND TUNING (~2900-3200, rough)
Goal: a stronger evaluation, and a data pipeline that feeds NNUE.
Tasks: tapered (midgame/endgame) eval with game phase interpolation;
terms for passed, isolated, doubled, and backward pawns, bishop pair,
rook on open files and the seventh rank, knight outposts, mobility, king
safety (pawn shield, attackers in the king zone), tempo bonus;
Texel tuning on labeled positions from game results, SPSA for search
parameters; self-play data generation.
Pitfalls: overfitting to the tuning set (validate on a held-out set), a
lower tuning error that does not translate into Elo (always confirm with
SPRT), and double counting overlapping terms.
Exit: the tuned evaluation shows an SPRT-confirmed gain over the untuned
one.

PHASE 8: NNUE EVALUATION (~3300+ Elo, rough)
Goal: the largest single strength jump for a modern engine.
Tasks:
 a. Learn the architecture: sparse input features (for example
    king-bucketed piece-square inputs), a wide first layer kept as an
    incrementally updated accumulator, then small dense layers with
    clipped ReLU.
 b. Start small: a modest hidden size (for example 128 to 768 neurons).
 c. Implement inference first: incremental accumulator updates on
    make/unmake, integer quantization, AVX2 SIMD.
 d. Choose a trainer (bullet, or nnue-pytorch; check each project's current
    documentation).
 e. Generate training data from self-play or search-labeled positions
    (result and score labels), hundreds of millions of positions for
    strong networks.
 f. Train, export, quantize, load into the engine, and verify that engine
    inference matches the trainer's output on test positions.
 g. Bootstrapping loop: use the improved engine to produce better data,
    retrain, and repeat.
Hardware notes: a GPU helps training a lot; small networks can be trained
on modest hardware, but the 5600H alone will be slow for big nets.
Pitfalls: accumulator not updated or restored correctly on unmake, wrong
quantization scales, perspective mix-ups between the two sides, and
training data leakage or low diversity.
Exit: the NNUE engine beats the best handcrafted-eval build by a large,
SPRT-verified margin at equal time.

PHASE 9: ENGINEERING AND SCALING
Goal: speed, stability, and good behavior in real matches.
Tasks: profile and optimize hot paths; TT prefetching; LTO and
profile-guided optimization; Lazy SMP (threads share the TT and search
with slight variation) and test thread scaling; time management (use
remaining time, increment, and moves to go; extend when the best move is
unstable or the score drops; stop early on forced moves); pondering if
wanted; Syzygy tablebase probing (for example via Fathom); robust UCI
handling and hash resizing.
Pitfalls: data races in shared structures, TT corruption under threads,
losing on time because of slow stop handling, and benchmark noise from
thermal throttling.
Exit: the engine scales with threads and hash size, never loses on time,
and runs stable for thousands of games.

PHASE 10: GAUNTLET TOWARD STOCKFISH
Goal: always know where the engine stands.
Ladder, in order:
 1. Small open-source engines (win a clear majority at equal time).
 2. Stronger open-source NNUE engines (reach parity, then win).
 3. Stockfish with UCI_LimitStrength and a low UCI_Elo, then higher
    limits.
 4. Stockfish at low fixed node counts (for example 1k to 100k nodes).
 5. Stockfish at short time control with a time handicap, shrinking the
    handicap over time.
 6. Stockfish at equal time and hardware, to measure the gap.
Rules: identical hardware, hash, and thread counts; paired balanced
openings; enough games for useful error margins. Analyze every loss by
finding where the evaluation diverged and whether search, eval, or time
management caused it. Optionally compare against public lists (CCRL,
CEGT) once stable.
Exit: a documented Elo estimate relative to Stockfish at several
handicaps, with error margins.

PHASE 11: CHALLENGE MATCH AGAINST STOCKFISH
Setup: same machine, threads, hash, and time control for both; balanced
paired openings; no pondering or opening books unless both use them
equally; thousands of games; record exact versions, build flags, and
settings so the result is reproducible.
Report: wins, draws, losses, score percentage, Elo difference with error
margin, game-pair statistics from fastchess, all PGNs, and the settings
log.
Expectation: expect to lose most /regames at full strength and equal time.
Success is measured by the handicap at which I reach parity and by how
that handicap shrinks over time.
GOAL LADDER (update as each is reached, with the date and result)
1. Beat Stockfish at UCI_Elo 2000, 2500, 3000 (equal time)
2. Beat Stockfish at 10k, 100k, 1M fixed nodes
3. Beat Stockfish with 10x, 5x, 3x, 2x time odds
4. Draw and win individual games at equal time
5. Match at equal time and hardware (long-term)