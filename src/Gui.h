#pragma once
#include <SFML/Graphics.hpp>
#include "ChessEngine.h"
#include "ChessBot.h"
#include <optional>
#include <future>
#include <atomic>

// ============================================================
// GUI - SFML-based chess board renderer and input handler
// ============================================================

class Gui {
public:
    static constexpr int BOARD_SIZE  = 8;
    static constexpr int CELL_SIZE   = 80;
    static constexpr int WINDOW_W    = CELL_SIZE * BOARD_SIZE;
    static constexpr int WINDOW_H    = CELL_SIZE * BOARD_SIZE + 60; // extra space for status bar

    Gui();

    // Main game loop - opens window and runs until closed
    void run();

private:
    sf::RenderWindow window_;
    sf::Font         font_;

    ChessEngine engine_;
    ChessBot    bot_;

    // Selection state
    bool hasSelection_    = false;
    int  selectedRow_     = -1;
    int  selectedCol_     = -1;
    std::vector<Move> legalMovesForSelected_;

    // Drag state
    bool isDragging_  = false;
    sf::Vector2f dragOffset_;
    sf::Vector2f dragPos_;

    // Bot state
    std::atomic<bool> botThinking_{false};
    std::future<Move> botFuture_;

    // Game status
    bool gameOver_  = false;
    std::string statusMessage_;

    // Promotion dialog
    bool showPromoDialog_ = false;
    Move pendingPromoMove_;

    // Colors
    sf::Color lightSquareColor_{240, 217, 181};
    sf::Color darkSquareColor_ {181, 136,  99};
    sf::Color selectedColor_   {100, 200, 100, 160};
    sf::Color legalMoveColor_  {100, 200, 100, 100};
    sf::Color lastMoveColor_   {205, 210,  50, 150};
    sf::Color checkColor_      {220,  50,  50, 180};

    // Last move for highlighting
    std::optional<Move> lastMove_;

    // Event handling
    void handleEvent(const sf::Event& event);
    void handleMousePress(int x, int y);
    void handleMouseRelease(int x, int y);
    void handleMouseMove(int x, int y);

    // Board coordinate conversion
    int pixelToRow(int y) const { return y / CELL_SIZE; }
    int pixelToCol(int x) const { return x / CELL_SIZE; }
    sf::Vector2f cellToPixel(int row, int col) const {
        return {static_cast<float>(col * CELL_SIZE), static_cast<float>(row * CELL_SIZE)};
    }

    // Drawing methods
    void draw();
    void drawBoard();
    void drawHighlights();
    void drawPieces();
    void drawPiece(const Piece& piece, float x, float y, float size, bool transparent = false);
    void drawStatus();
    void drawPromoDialog();
    void drawDraggedPiece();

    // Unicode chess symbols
    static sf::String pieceToUnicode(const Piece& piece);

    // Game logic
    void selectSquare(int row, int col);
    void tryMove(int toRow, int toCol);
    void executeMove(const Move& move);
    void startBotMove();
    void checkBotMove();
    void updateStatus();

    // Promotion dialog handling (event-driven, called from handleEvent)
    void handlePromoClick(int x, int y);
};
