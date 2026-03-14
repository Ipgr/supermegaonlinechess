#include "ChessBot.h"
#include <algorithm>
#include <limits>

// ============================================================
// Piece-Square Tables (from White's perspective, row 0 = rank 8)
// For Black, we mirror the table vertically (row 7-row)
// ============================================================

// Pawns: encourage advancement and center control
static const int PST_PAWN[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },
    { 50, 50, 50, 50, 50, 50, 50, 50 },
    { 10, 10, 20, 30, 30, 20, 10, 10 },
    {  5,  5, 10, 25, 25, 10,  5,  5 },
    {  0,  0,  0, 20, 20,  0,  0,  0 },
    {  5, -5,-10,  0,  0,-10, -5,  5 },
    {  5, 10, 10,-20,-20, 10, 10,  5 },
    {  0,  0,  0,  0,  0,  0,  0,  0 }
};

// Knights: bonus for central squares, penalty for edges
static const int PST_KNIGHT[8][8] = {
    {-50,-40,-30,-30,-30,-30,-40,-50 },
    {-40,-20,  0,  0,  0,  0,-20,-40 },
    {-30,  0, 10, 15, 15, 10,  0,-30 },
    {-30,  5, 15, 20, 20, 15,  5,-30 },
    {-30,  0, 15, 20, 20, 15,  0,-30 },
    {-30,  5, 10, 15, 15, 10,  5,-30 },
    {-40,-20,  0,  5,  5,  0,-20,-40 },
    {-50,-40,-30,-30,-30,-30,-40,-50 }
};

// Bishops: prefer long diagonals
static const int PST_BISHOP[8][8] = {
    {-20,-10,-10,-10,-10,-10,-10,-20 },
    {-10,  0,  0,  0,  0,  0,  0,-10 },
    {-10,  0,  5, 10, 10,  5,  0,-10 },
    {-10,  5,  5, 10, 10,  5,  5,-10 },
    {-10,  0, 10, 10, 10, 10,  0,-10 },
    {-10, 10, 10, 10, 10, 10, 10,-10 },
    {-10,  5,  0,  0,  0,  0,  5,-10 },
    {-20,-10,-10,-10,-10,-10,-10,-20 }
};

// Rooks: open files, 7th rank
static const int PST_ROOK[8][8] = {
    {  0,  0,  0,  0,  0,  0,  0,  0 },
    {  5, 10, 10, 10, 10, 10, 10,  5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    { -5,  0,  0,  0,  0,  0,  0, -5 },
    {  0,  0,  0,  5,  5,  0,  0,  0 }
};

// Queen: flexible positioning
static const int PST_QUEEN[8][8] = {
    {-20,-10,-10, -5, -5,-10,-10,-20 },
    {-10,  0,  0,  0,  0,  0,  0,-10 },
    {-10,  0,  5,  5,  5,  5,  0,-10 },
    { -5,  0,  5,  5,  5,  5,  0, -5 },
    {  0,  0,  5,  5,  5,  5,  0, -5 },
    {-10,  5,  5,  5,  5,  5,  0,-10 },
    {-10,  0,  5,  0,  0,  0,  0,-10 },
    {-20,-10,-10, -5, -5,-10,-10,-20 }
};

// King middle-game: safety behind pawns
static const int PST_KING_MG[8][8] = {
    {-30,-40,-40,-50,-50,-40,-40,-30 },
    {-30,-40,-40,-50,-50,-40,-40,-30 },
    {-30,-40,-40,-50,-50,-40,-40,-30 },
    {-30,-40,-40,-50,-50,-40,-40,-30 },
    {-20,-30,-30,-40,-40,-30,-30,-20 },
    {-10,-20,-20,-20,-20,-20,-20,-10 },
    { 20, 20,  0,  0,  0,  0, 20, 20 },
    { 20, 30, 10,  0,  0, 10, 30, 20 }
};

// ============================================================
// Constructor
// ============================================================

ChessBot::ChessBot(int depth) : depth_(depth) {}

// ============================================================
// Piece values
// ============================================================

int ChessBot::pieceValue(PieceType type) {
    switch (type) {
        case PieceType::Pawn:   return 100;
        case PieceType::Knight: return 320;
        case PieceType::Bishop: return 330;
        case PieceType::Rook:   return 500;
        case PieceType::Queen:  return 900;
        case PieceType::King:   return 20000;
        default: return 0;
    }
}

// ============================================================
// PST lookup
// ============================================================

int ChessBot::pstValue(PieceType type, Color color, int row, int col) {
    // For White: use table as-is (row 0 is opponent's side, row 7 is our side)
    // For Black: mirror vertically
    int r = (color == Color::White) ? row : (7 - row);

    switch (type) {
        case PieceType::Pawn:   return PST_PAWN[r][col];
        case PieceType::Knight: return PST_KNIGHT[r][col];
        case PieceType::Bishop: return PST_BISHOP[r][col];
        case PieceType::Rook:   return PST_ROOK[r][col];
        case PieceType::Queen:  return PST_QUEEN[r][col];
        case PieceType::King:   return PST_KING_MG[r][col];
        default: return 0;
    }
}

// ============================================================
// Static evaluation
// ============================================================

int ChessBot::evaluate(const BoardState& state) const {
    int score = 0;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& p = state.board[r][c];
            if (p.isEmpty()) continue;
            int val = pieceValue(p.type) + pstValue(p.type, p.color, r, c);
            if (p.color == Color::White) score += val;
            else score -= val;
        }
    }
    return score;
}

// ============================================================
// Move ordering
// ============================================================

int ChessBot::scoreMoveForOrdering(const Move& move, const BoardState& state) const {
    int score = 0;
    const Piece& moving = state.board[move.fromRow][move.fromCol];
    const Piece& target = state.board[move.toRow][move.toCol];

    // Captures: MVV-LVA (Most Valuable Victim - Least Valuable Attacker)
    if (!target.isEmpty()) {
        score += 10 * pieceValue(target.type) - pieceValue(moving.type);
        score += 10000; // prioritize captures
    }

    // Promotions
    if (move.type == MoveType::Promotion) {
        score += pieceValue(move.promotionPiece) + 5000;
    }

    // En passant
    if (move.type == MoveType::EnPassant) {
        score += 100 + 8000;
    }

    return score;
}

void ChessBot::sortMoves(std::vector<Move>& moves, const BoardState& state) const {
    std::stable_sort(moves.begin(), moves.end(), [&](const Move& a, const Move& b) {
        return scoreMoveForOrdering(a, state) > scoreMoveForOrdering(b, state);
    });
}

// ============================================================
// Minimax with Alpha-Beta Pruning
// ============================================================

int ChessBot::minimax(ChessEngine& engine, int depth, int alpha, int beta, bool maximizing) const {
    if (depth == 0) {
        return evaluate(engine.getState());
    }

    const BoardState& st = engine.getState();
    Color currentColor = st.currentTurn;
    auto moves = engine.generateLegalMoves();

    if (moves.empty()) {
        // Check for checkmate or stalemate
        if (engine.isInCheck(currentColor)) {
            // Checkmate: return heavily biased score (worse for the checkmated side)
            return maximizing ? -100000 - depth : 100000 + depth;
        } else {
            // Stalemate: draw
            return 0;
        }
    }

    // Sort moves for better pruning
    sortMoves(moves, st);

    if (maximizing) {
        int maxEval = std::numeric_limits<int>::min();
        for (const auto& move : moves) {
            BoardState saved = engine.getState();
            engine.applyMove(move);
            int eval = minimax(engine, depth - 1, alpha, beta, false);
            engine.getState() = saved;
            maxEval = std::max(maxEval, eval);
            alpha = std::max(alpha, eval);
            if (beta <= alpha) break; // Beta cutoff
        }
        return maxEval;
    } else {
        int minEval = std::numeric_limits<int>::max();
        for (const auto& move : moves) {
            BoardState saved = engine.getState();
            engine.applyMove(move);
            int eval = minimax(engine, depth - 1, alpha, beta, true);
            engine.getState() = saved;
            minEval = std::min(minEval, eval);
            beta = std::min(beta, eval);
            if (beta <= alpha) break; // Alpha cutoff
        }
        return minEval;
    }
}

// ============================================================
// Find best move
// ============================================================

Move ChessBot::findBestMove(ChessEngine& engine, Color color) {
    auto moves = engine.generateLegalMoves();
    if (moves.empty()) {
        return Move{}; // no moves available
    }

    // Sort moves for better pruning
    sortMoves(moves, engine.getState());

    Move bestMove = moves[0];
    bool maximizing = (color == Color::White);

    int bestScore = maximizing ? std::numeric_limits<int>::min()
                               : std::numeric_limits<int>::max();

    int alpha = std::numeric_limits<int>::min();
    int beta  = std::numeric_limits<int>::max();

    for (const auto& move : moves) {
        BoardState saved = engine.getState();
        engine.applyMove(move);
        int score = minimax(engine, depth_ - 1, alpha, beta, !maximizing);
        engine.getState() = saved;

        if (maximizing && score > bestScore) {
            bestScore = score;
            bestMove = move;
            alpha = std::max(alpha, score);
        } else if (!maximizing && score < bestScore) {
            bestScore = score;
            bestMove = move;
            beta = std::min(beta, score);
        }
    }

    return bestMove;
}
