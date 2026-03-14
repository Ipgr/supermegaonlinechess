#pragma once
#include <vector>
#include <cstdint>
#include <optional>

// ============================================================
// Piece types and colors
// ============================================================

enum class PieceType : uint8_t {
    None = 0,
    Pawn,
    Knight,
    Bishop,
    Rook,
    Queen,
    King
};

enum class Color : uint8_t {
    None = 0,
    White,
    Black
};

struct Piece {
    PieceType type = PieceType::None;
    Color     color = Color::None;

    bool isEmpty() const { return type == PieceType::None; }
};

// ============================================================
// Move representation
// ============================================================

enum class MoveType : uint8_t {
    Normal = 0,
    Castling,
    EnPassant,
    Promotion
};

struct Move {
    int fromRow = 0, fromCol = 0;
    int toRow   = 0, toCol   = 0;
    MoveType type = MoveType::Normal;
    PieceType promotionPiece = PieceType::Queen; // used when type == Promotion

    bool operator==(const Move& o) const {
        return fromRow == o.fromRow && fromCol == o.fromCol &&
               toRow   == o.toRow   && toCol   == o.toCol   &&
               type    == o.type    && promotionPiece == o.promotionPiece;
    }
};

// ============================================================
// Board state
// ============================================================

struct BoardState {
    Piece board[8][8];

    // Castling rights
    bool whiteKingMoved  = false;
    bool blackKingMoved  = false;
    bool whiteRookAMoved = false; // a1 rook (queen-side)
    bool whiteRookHMoved = false; // h1 rook (king-side)
    bool blackRookAMoved = false; // a8 rook
    bool blackRookHMoved = false; // h8 rook

    // En passant target square (-1 if none)
    int enPassantCol = -1; // column where en passant capture is possible
    int enPassantRow = -1; // row of the pawn that can be captured

    Color currentTurn = Color::White;
};

// ============================================================
// ChessEngine class
// ============================================================

class ChessEngine {
public:
    ChessEngine();

    // Initialize board to starting position
    void initBoard();

    // Get current board state
    const BoardState& getState() const { return state_; }
    BoardState& getState() { return state_; }

    // Generate all pseudo-legal moves for a piece at (row, col)
    std::vector<Move> generateMovesForPiece(int row, int col) const;

    // Generate all legal moves for the current player
    std::vector<Move> generateLegalMoves() const;

    // Generate all legal moves for the specified color
    std::vector<Move> generateLegalMovesForColor(Color color) const;

    // Apply a move (modifies internal state)
    void applyMove(const Move& move);

    // Apply move to a given state (returns new state, does not modify internal)
    BoardState applyMoveToState(const BoardState& st, const Move& move) const;

    // Check if the given color's king is in check
    bool isInCheck(Color color) const;
    bool isInCheckInState(const BoardState& st, Color color) const;

    // Check for checkmate / stalemate
    bool isCheckmate() const;
    bool isStalemate() const;

    // Determine game over status
    bool isGameOver() const;

    // Get the winner (Color::None if draw/not over)
    Color getWinner() const;

    // Check if a square is attacked by the given color
    bool isSquareAttacked(int row, int col, Color byColor) const;
    bool isSquareAttackedInState(const BoardState& st, int row, int col, Color byColor) const;

    // Find king position for a color
    std::pair<int,int> findKing(Color color) const;
    std::pair<int,int> findKingInState(const BoardState& st, Color color) const;

private:
    BoardState state_;

    // Raw move generation (pseudo-legal, ignores check)
    std::vector<Move> generatePseudoLegalMoves(const BoardState& st, int row, int col) const;
    void generatePawnMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const;
    void generateKnightMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const;
    void generateBishopMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const;
    void generateRookMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const;
    void generateQueenMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const;
    void generateKingMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const;

    // Filter pseudo-legal moves to only legal moves
    std::vector<Move> filterLegalMoves(const BoardState& st, const std::vector<Move>& pseudoLegal, Color color) const;
};
