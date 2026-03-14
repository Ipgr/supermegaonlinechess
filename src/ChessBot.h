#pragma once
#include "ChessEngine.h"

// ============================================================
// ChessBot - AI using Minimax with Alpha-Beta Pruning
// ============================================================

class ChessBot {
public:
    // Depth of search (half-moves / plies)
    static constexpr int DEFAULT_DEPTH = 4;

    explicit ChessBot(int depth = DEFAULT_DEPTH);

    // Find the best move for the given color using current engine state
    Move findBestMove(ChessEngine& engine, Color color);

    // Static position evaluation (positive = White advantage, negative = Black advantage)
    int evaluate(const BoardState& state) const;

    void setDepth(int d) { depth_ = d; }
    int  getDepth() const { return depth_; }

private:
    int depth_;

    // Minimax with alpha-beta pruning
    // Returns evaluation score from White's perspective
    int minimax(ChessEngine& engine, int depth, int alpha, int beta, bool maximizing) const;

    // Sort moves for better alpha-beta efficiency (captures first, then checks)
    void sortMoves(std::vector<Move>& moves, const BoardState& state) const;

    // Score a single move for ordering
    int scoreMoveForOrdering(const Move& move, const BoardState& state) const;

    // Material value of a piece type
    static int pieceValue(PieceType type);

    // Piece-Square Table lookup
    static int pstValue(PieceType type, Color color, int row, int col);
};
