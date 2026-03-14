#include "ChessEngine.h"
#include <algorithm>
#include <stdexcept>

// ============================================================
// Constructor
// ============================================================

ChessEngine::ChessEngine() {
    initBoard();
}

// ============================================================
// Board initialization
// ============================================================

void ChessEngine::initBoard() {
    // Clear board
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            state_.board[r][c] = Piece{};

    // Piece layout (row 0 = rank 8 for black, row 7 = rank 1 for white)
    auto placePiece = [&](int row, int col, PieceType type, Color color) {
        state_.board[row][col] = Piece{type, color};
    };

    // Black pieces (top)
    placePiece(0, 0, PieceType::Rook,   Color::Black);
    placePiece(0, 1, PieceType::Knight, Color::Black);
    placePiece(0, 2, PieceType::Bishop, Color::Black);
    placePiece(0, 3, PieceType::Queen,  Color::Black);
    placePiece(0, 4, PieceType::King,   Color::Black);
    placePiece(0, 5, PieceType::Bishop, Color::Black);
    placePiece(0, 6, PieceType::Knight, Color::Black);
    placePiece(0, 7, PieceType::Rook,   Color::Black);
    for (int c = 0; c < 8; ++c)
        placePiece(1, c, PieceType::Pawn, Color::Black);

    // White pieces (bottom)
    placePiece(7, 0, PieceType::Rook,   Color::White);
    placePiece(7, 1, PieceType::Knight, Color::White);
    placePiece(7, 2, PieceType::Bishop, Color::White);
    placePiece(7, 3, PieceType::Queen,  Color::White);
    placePiece(7, 4, PieceType::King,   Color::White);
    placePiece(7, 5, PieceType::Bishop, Color::White);
    placePiece(7, 6, PieceType::Knight, Color::White);
    placePiece(7, 7, PieceType::Rook,   Color::White);
    for (int c = 0; c < 8; ++c)
        placePiece(6, c, PieceType::Pawn, Color::White);

    // Reset game state
    state_.whiteKingMoved  = false;
    state_.blackKingMoved  = false;
    state_.whiteRookAMoved = false;
    state_.whiteRookHMoved = false;
    state_.blackRookAMoved = false;
    state_.blackRookHMoved = false;
    state_.enPassantCol    = -1;
    state_.enPassantRow    = -1;
    state_.currentTurn     = Color::White;
}

// ============================================================
// Move generation helpers
// ============================================================

static bool inBounds(int r, int c) {
    return r >= 0 && r < 8 && c >= 0 && c < 8;
}

void ChessEngine::generatePawnMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const {
    Color color = st.board[row][col].color;
    int dir = (color == Color::White) ? -1 : 1; // white moves up (decreasing row), black moves down
    int startRow = (color == Color::White) ? 6 : 1;
    int promRow  = (color == Color::White) ? 0 : 7;

    auto addMove = [&](int tr, int tc, MoveType mt = MoveType::Normal) {
        if (tr == promRow) {
            // Add all promotion options
            for (auto pt : {PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight}) {
                moves.push_back({row, col, tr, tc, MoveType::Promotion, pt});
            }
        } else {
            moves.push_back({row, col, tr, tc, mt});
        }
    };

    // Single step forward
    int nr = row + dir;
    if (inBounds(nr, col) && st.board[nr][col].isEmpty()) {
        addMove(nr, col);
        // Double step from starting position
        if (row == startRow) {
            int nr2 = row + 2 * dir;
            if (inBounds(nr2, col) && st.board[nr2][col].isEmpty()) {
                addMove(nr2, col);
            }
        }
    }

    // Diagonal captures
    for (int dc : {-1, 1}) {
        int nc = col + dc;
        if (!inBounds(nr, nc)) continue;
        // Normal capture
        if (!st.board[nr][nc].isEmpty() && st.board[nr][nc].color != color) {
            addMove(nr, nc);
        }
        // En passant
        if (st.enPassantCol == nc && st.enPassantRow == row) {
            // The pawn to capture is at (row, nc), we move to (nr, nc)
            moves.push_back({row, col, nr, nc, MoveType::EnPassant});
        }
    }
}

void ChessEngine::generateKnightMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const {
    Color color = st.board[row][col].color;
    static const int deltas[8][2] = {{-2,-1},{-2,1},{-1,-2},{-1,2},{1,-2},{1,2},{2,-1},{2,1}};
    for (auto& d : deltas) {
        int nr = row + d[0], nc = col + d[1];
        if (inBounds(nr, nc) && (st.board[nr][nc].isEmpty() || st.board[nr][nc].color != color)) {
            moves.push_back({row, col, nr, nc});
        }
    }
}

void ChessEngine::generateBishopMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const {
    Color color = st.board[row][col].color;
    static const int dirs[4][2] = {{-1,-1},{-1,1},{1,-1},{1,1}};
    for (auto& d : dirs) {
        int nr = row + d[0], nc = col + d[1];
        while (inBounds(nr, nc)) {
            if (st.board[nr][nc].isEmpty()) {
                moves.push_back({row, col, nr, nc});
            } else {
                if (st.board[nr][nc].color != color)
                    moves.push_back({row, col, nr, nc});
                break;
            }
            nr += d[0]; nc += d[1];
        }
    }
}

void ChessEngine::generateRookMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const {
    Color color = st.board[row][col].color;
    static const int dirs[4][2] = {{-1,0},{1,0},{0,-1},{0,1}};
    for (auto& d : dirs) {
        int nr = row + d[0], nc = col + d[1];
        while (inBounds(nr, nc)) {
            if (st.board[nr][nc].isEmpty()) {
                moves.push_back({row, col, nr, nc});
            } else {
                if (st.board[nr][nc].color != color)
                    moves.push_back({row, col, nr, nc});
                break;
            }
            nr += d[0]; nc += d[1];
        }
    }
}

void ChessEngine::generateQueenMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const {
    generateBishopMoves(st, row, col, moves);
    generateRookMoves(st, row, col, moves);
}

void ChessEngine::generateKingMoves(const BoardState& st, int row, int col, std::vector<Move>& moves) const {
    Color color = st.board[row][col].color;
    static const int dirs[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
    for (auto& d : dirs) {
        int nr = row + d[0], nc = col + d[1];
        if (inBounds(nr, nc) && (st.board[nr][nc].isEmpty() || st.board[nr][nc].color != color)) {
            moves.push_back({row, col, nr, nc});
        }
    }

    // Castling
    bool kingMoved = (color == Color::White) ? st.whiteKingMoved : st.blackKingMoved;
    if (!kingMoved) {
        int kingRow = (color == Color::White) ? 7 : 0;
        if (row != kingRow || col != 4) return; // king not on original square

        bool rookAMoved = (color == Color::White) ? st.whiteRookAMoved : st.blackRookAMoved;
        bool rookHMoved = (color == Color::White) ? st.whiteRookHMoved : st.blackRookHMoved;

        Color opp = (color == Color::White) ? Color::Black : Color::White;

        // King-side castling (short castling)
        if (!rookHMoved &&
            st.board[kingRow][5].isEmpty() &&
            st.board[kingRow][6].isEmpty() &&
            st.board[kingRow][7].type == PieceType::Rook &&
            st.board[kingRow][7].color == color &&
            !isSquareAttackedInState(st, kingRow, 4, opp) &&
            !isSquareAttackedInState(st, kingRow, 5, opp) &&
            !isSquareAttackedInState(st, kingRow, 6, opp))
        {
            moves.push_back({row, col, kingRow, 6, MoveType::Castling});
        }

        // Queen-side castling (long castling)
        if (!rookAMoved &&
            st.board[kingRow][3].isEmpty() &&
            st.board[kingRow][2].isEmpty() &&
            st.board[kingRow][1].isEmpty() &&
            st.board[kingRow][0].type == PieceType::Rook &&
            st.board[kingRow][0].color == color &&
            !isSquareAttackedInState(st, kingRow, 4, opp) &&
            !isSquareAttackedInState(st, kingRow, 3, opp) &&
            !isSquareAttackedInState(st, kingRow, 2, opp))
        {
            moves.push_back({row, col, kingRow, 2, MoveType::Castling});
        }
    }
}

// ============================================================
// Pseudo-legal move generation
// ============================================================

std::vector<Move> ChessEngine::generatePseudoLegalMoves(const BoardState& st, int row, int col) const {
    std::vector<Move> moves;
    if (st.board[row][col].isEmpty()) return moves;

    switch (st.board[row][col].type) {
        case PieceType::Pawn:   generatePawnMoves(st, row, col, moves); break;
        case PieceType::Knight: generateKnightMoves(st, row, col, moves); break;
        case PieceType::Bishop: generateBishopMoves(st, row, col, moves); break;
        case PieceType::Rook:   generateRookMoves(st, row, col, moves); break;
        case PieceType::Queen:  generateQueenMoves(st, row, col, moves); break;
        case PieceType::King:   generateKingMoves(st, row, col, moves); break;
        default: break;
    }
    return moves;
}

// ============================================================
// Move application
// ============================================================

BoardState ChessEngine::applyMoveToState(const BoardState& st, const Move& move) const {
    BoardState ns = st;
    Piece moving = ns.board[move.fromRow][move.fromCol];

    // Clear en passant
    ns.enPassantCol = -1;
    ns.enPassantRow = -1;

    switch (move.type) {
        case MoveType::Normal: {
            // Track castling rights
            if (moving.type == PieceType::King) {
                if (moving.color == Color::White) ns.whiteKingMoved = true;
                else ns.blackKingMoved = true;
            }
            if (moving.type == PieceType::Rook) {
                if (moving.color == Color::White) {
                    if (move.fromRow == 7 && move.fromCol == 0) ns.whiteRookAMoved = true;
                    if (move.fromRow == 7 && move.fromCol == 7) ns.whiteRookHMoved = true;
                } else {
                    if (move.fromRow == 0 && move.fromCol == 0) ns.blackRookAMoved = true;
                    if (move.fromRow == 0 && move.fromCol == 7) ns.blackRookHMoved = true;
                }
            }
            // Track if rook is captured (invalidate castling rights)
            Piece& target = ns.board[move.toRow][move.toCol];
            if (target.type == PieceType::Rook) {
                if (target.color == Color::White) {
                    if (move.toRow == 7 && move.toCol == 0) ns.whiteRookAMoved = true;
                    if (move.toRow == 7 && move.toCol == 7) ns.whiteRookHMoved = true;
                } else {
                    if (move.toRow == 0 && move.toCol == 0) ns.blackRookAMoved = true;
                    if (move.toRow == 0 && move.toCol == 7) ns.blackRookHMoved = true;
                }
            }
            // Double pawn push: set en passant
            if (moving.type == PieceType::Pawn && std::abs(move.toRow - move.fromRow) == 2) {
                ns.enPassantCol = move.fromCol;
                ns.enPassantRow = move.toRow; // row of the pawn that just moved
            }
            ns.board[move.toRow][move.toCol] = moving;
            ns.board[move.fromRow][move.fromCol] = Piece{};
            break;
        }
        case MoveType::Castling: {
            if (moving.color == Color::White) ns.whiteKingMoved = true;
            else ns.blackKingMoved = true;

            ns.board[move.toRow][move.toCol] = moving;
            ns.board[move.fromRow][move.fromCol] = Piece{};

            // Move rook
            int kingRow = move.toRow;
            if (move.toCol == 6) { // King-side
                ns.board[kingRow][5] = ns.board[kingRow][7];
                ns.board[kingRow][7] = Piece{};
                if (moving.color == Color::White) ns.whiteRookHMoved = true;
                else ns.blackRookHMoved = true;
            } else { // Queen-side (col 2)
                ns.board[kingRow][3] = ns.board[kingRow][0];
                ns.board[kingRow][0] = Piece{};
                if (moving.color == Color::White) ns.whiteRookAMoved = true;
                else ns.blackRookAMoved = true;
            }
            break;
        }
        case MoveType::EnPassant: {
            ns.board[move.toRow][move.toCol] = moving;
            ns.board[move.fromRow][move.fromCol] = Piece{};
            // Remove the captured pawn
            ns.board[move.fromRow][move.toCol] = Piece{};
            break;
        }
        case MoveType::Promotion: {
            // If a rook is captured during promotion, invalidate that rook's castling rights
            const Piece& capturedByPromo = ns.board[move.toRow][move.toCol];
            if (capturedByPromo.type == PieceType::Rook) {
                if (capturedByPromo.color == Color::White) {
                    if (move.toRow == 7 && move.toCol == 0) ns.whiteRookAMoved = true;
                    if (move.toRow == 7 && move.toCol == 7) ns.whiteRookHMoved = true;
                } else {
                    if (move.toRow == 0 && move.toCol == 0) ns.blackRookAMoved = true;
                    if (move.toRow == 0 && move.toCol == 7) ns.blackRookHMoved = true;
                }
            }
            ns.board[move.toRow][move.toCol] = Piece{move.promotionPiece, moving.color};
            ns.board[move.fromRow][move.fromCol] = Piece{};
            break;
        }
    }

    // Switch turn
    ns.currentTurn = (ns.currentTurn == Color::White) ? Color::Black : Color::White;
    return ns;
}

void ChessEngine::applyMove(const Move& move) {
    state_ = applyMoveToState(state_, move);
}

// ============================================================
// Legal move generation
// ============================================================

std::vector<Move> ChessEngine::filterLegalMoves(const BoardState& st, const std::vector<Move>& pseudoLegal, Color color) const {
    std::vector<Move> legal;
    legal.reserve(pseudoLegal.size());
    for (const auto& m : pseudoLegal) {
        BoardState ns = applyMoveToState(st, m);
        // After applying move, it's the opponent's turn; check if original color is in check
        if (!isInCheckInState(ns, color)) {
            legal.push_back(m);
        }
    }
    return legal;
}

std::vector<Move> ChessEngine::generateMovesForPiece(int row, int col) const {
    if (state_.board[row][col].isEmpty()) return {};
    Color color = state_.board[row][col].color;
    auto pseudo = generatePseudoLegalMoves(state_, row, col);
    return filterLegalMoves(state_, pseudo, color);
}

std::vector<Move> ChessEngine::generateLegalMoves() const {
    return generateLegalMovesForColor(state_.currentTurn);
}

std::vector<Move> ChessEngine::generateLegalMovesForColor(Color color) const {
    std::vector<Move> all;
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (!state_.board[r][c].isEmpty() && state_.board[r][c].color == color) {
                auto pseudo = generatePseudoLegalMoves(state_, r, c);
                auto legal = filterLegalMoves(state_, pseudo, color);
                all.insert(all.end(), legal.begin(), legal.end());
            }
        }
    }
    return all;
}

// ============================================================
// Attack detection
// ============================================================

bool ChessEngine::isSquareAttacked(int row, int col, Color byColor) const {
    return isSquareAttackedInState(state_, row, col, byColor);
}

bool ChessEngine::isSquareAttackedInState(const BoardState& st, int row, int col, Color byColor) const {
    // Check all pieces of byColor to see if they attack (row, col)
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            if (st.board[r][c].isEmpty() || st.board[r][c].color != byColor) continue;
            // Generate pseudo-legal moves for this piece and see if any land on (row, col)
            std::vector<Move> moves;
            // For pawns, only consider diagonal attacks (not forward moves)
            if (st.board[r][c].type == PieceType::Pawn) {
                int dir = (byColor == Color::White) ? -1 : 1;
                for (int dc : {-1, 1}) {
                    if (r + dir == row && c + dc == col) return true;
                }
                continue;
            }
            switch (st.board[r][c].type) {
                case PieceType::Knight: generateKnightMoves(st, r, c, moves); break;
                case PieceType::Bishop: generateBishopMoves(st, r, c, moves); break;
                case PieceType::Rook:   generateRookMoves(st, r, c, moves); break;
                case PieceType::Queen:  generateQueenMoves(st, r, c, moves); break;
                case PieceType::King: {
                    static const int kd[8][2] = {{-1,-1},{-1,0},{-1,1},{0,-1},{0,1},{1,-1},{1,0},{1,1}};
                    for (auto& d : kd) {
                        if (r+d[0] == row && c+d[1] == col) return true;
                    }
                    continue;
                }
                default: continue;
            }
            for (const auto& m : moves) {
                if (m.toRow == row && m.toCol == col) return true;
            }
        }
    }
    return false;
}

// ============================================================
// King finding
// ============================================================

std::pair<int,int> ChessEngine::findKing(Color color) const {
    return findKingInState(state_, color);
}

std::pair<int,int> ChessEngine::findKingInState(const BoardState& st, Color color) const {
    for (int r = 0; r < 8; ++r)
        for (int c = 0; c < 8; ++c)
            if (st.board[r][c].type == PieceType::King && st.board[r][c].color == color)
                return {r, c};
    return {-1, -1};
}

// ============================================================
// Check, checkmate, stalemate
// ============================================================

bool ChessEngine::isInCheck(Color color) const {
    return isInCheckInState(state_, color);
}

bool ChessEngine::isInCheckInState(const BoardState& st, Color color) const {
    auto [kr, kc] = findKingInState(st, color);
    if (kr < 0) return false;
    Color opp = (color == Color::White) ? Color::Black : Color::White;
    return isSquareAttackedInState(st, kr, kc, opp);
}

bool ChessEngine::isCheckmate() const {
    if (!isInCheck(state_.currentTurn)) return false;
    return generateLegalMoves().empty();
}

bool ChessEngine::isStalemate() const {
    if (isInCheck(state_.currentTurn)) return false;
    return generateLegalMoves().empty();
}

bool ChessEngine::isGameOver() const {
    return isCheckmate() || isStalemate();
}

Color ChessEngine::getWinner() const {
    if (isCheckmate()) {
        // Current player is in checkmate, so the other player won
        return (state_.currentTurn == Color::White) ? Color::Black : Color::White;
    }
    return Color::None;
}
