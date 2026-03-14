#include "Gui.h"
#include <sstream>
#include <stdexcept>

// ============================================================
// Constructor
// ============================================================

Gui::Gui()
    : window_(sf::VideoMode(WINDOW_W, WINDOW_H), "SuperMega Chess",
              sf::Style::Titlebar | sf::Style::Close)
    , bot_(ChessBot::DEFAULT_DEPTH)
{
    window_.setFramerateLimit(60);

    // Try to load a system font for text rendering
    if (!font_.loadFromFile("/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf") &&
        !font_.loadFromFile("/usr/share/fonts/TTF/DejaVuSans.ttf") &&
        !font_.loadFromFile("/usr/share/fonts/truetype/liberation/LiberationSans-Regular.ttf") &&
        !font_.loadFromFile("/usr/share/fonts/opentype/noto/NotoSans-Regular.ttf") &&
        !font_.loadFromFile("/usr/share/fonts/truetype/ubuntu/Ubuntu-R.ttf"))
    {
        // Font not found - we'll draw pieces as colored shapes
    }
}

// ============================================================
// Main game loop
// ============================================================

void Gui::run() {
    updateStatus();

    while (window_.isOpen()) {
        // Poll events
        sf::Event event;
        while (window_.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window_.close();
            else
                handleEvent(event);
        }

        // Check if bot has finished thinking
        if (botThinking_) {
            checkBotMove();
        }

        // Render
        window_.clear(sf::Color(40, 40, 40));
        draw();
        window_.display();
    }
}

// ============================================================
// Event handling
// ============================================================

void Gui::handleEvent(const sf::Event& event) {
    if (gameOver_ || botThinking_) return;

    if (event.type == sf::Event::MouseButtonPressed &&
        event.mouseButton.button == sf::Mouse::Left)
    {
        // Promotion dialog takes priority
        if (showPromoDialog_) {
            handlePromoClick(event.mouseButton.x, event.mouseButton.y);
            return;
        }
    }

    // Human plays White; only allow input when it's White's turn
    if (engine_.getState().currentTurn != Color::White) return;

    if (event.type == sf::Event::MouseButtonPressed) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            handleMousePress(event.mouseButton.x, event.mouseButton.y);
        }
    } else if (event.type == sf::Event::MouseButtonReleased) {
        if (event.mouseButton.button == sf::Mouse::Left) {
            handleMouseRelease(event.mouseButton.x, event.mouseButton.y);
        }
    } else if (event.type == sf::Event::MouseMoved) {
        handleMouseMove(event.mouseMove.x, event.mouseMove.y);
    }
}

void Gui::handleMousePress(int x, int y) {
    // Handle promotion dialog
    if (showPromoDialog_) return;

    int row = pixelToRow(y);
    int col = pixelToCol(x);
    if (row < 0 || row >= 8 || col < 0 || col >= 8) return;

    const Piece& p = engine_.getState().board[row][col];

    if (!hasSelection_) {
        // Select a piece
        if (!p.isEmpty() && p.color == Color::White) {
            selectedRow_ = row;
            selectedCol_ = col;
            hasSelection_ = true;
            legalMovesForSelected_ = engine_.generateMovesForPiece(row, col);
            // Start drag
            isDragging_  = true;
            dragPos_     = {static_cast<float>(x), static_cast<float>(y)};
            dragOffset_  = {static_cast<float>(x - col * CELL_SIZE),
                            static_cast<float>(y - row * CELL_SIZE)};
        }
    } else {
        // If clicking on own piece, re-select
        if (!p.isEmpty() && p.color == Color::White) {
            selectedRow_ = row;
            selectedCol_ = col;
            legalMovesForSelected_ = engine_.generateMovesForPiece(row, col);
            isDragging_  = true;
            dragPos_     = {static_cast<float>(x), static_cast<float>(y)};
            dragOffset_  = {static_cast<float>(x - col * CELL_SIZE),
                            static_cast<float>(y - row * CELL_SIZE)};
        } else {
            // Try to move to this square
            tryMove(row, col);
        }
    }
}

void Gui::handleMouseRelease(int x, int y) {
    if (!isDragging_) return;

    int row = pixelToRow(y);
    int col = pixelToCol(x);

    isDragging_ = false;

    if (row < 0 || row >= 8 || col < 0 || col >= 8) return;
    if (!hasSelection_) return;

    // If released on a different square, try to move there
    if (row != selectedRow_ || col != selectedCol_) {
        tryMove(row, col);
    }
}

void Gui::handleMouseMove(int x, int y) {
    if (isDragging_) {
        dragPos_ = {static_cast<float>(x), static_cast<float>(y)};
    }
}

// ============================================================
// Game logic
// ============================================================

void Gui::selectSquare(int row, int col) {
    hasSelection_ = true;
    selectedRow_  = row;
    selectedCol_  = col;
    legalMovesForSelected_ = engine_.generateMovesForPiece(row, col);
}

void Gui::tryMove(int toRow, int toCol) {
    // Find matching legal move
    for (const auto& m : legalMovesForSelected_) {
        if (m.toRow == toRow && m.toCol == toCol) {
            if (m.type == MoveType::Promotion) {
                // Show promotion dialog
                showPromoDialog_  = true;
                pendingPromoMove_ = m; // will pick promotion piece interactively
                hasSelection_     = false;
                legalMovesForSelected_.clear();
                isDragging_ = false;
                return;
            }
            executeMove(m);
            return;
        }
    }
    // Invalid move - deselect
    hasSelection_ = false;
    legalMovesForSelected_.clear();
}

void Gui::executeMove(const Move& move) {
    engine_.applyMove(move);
    lastMove_     = move;
    hasSelection_ = false;
    legalMovesForSelected_.clear();
    isDragging_   = false;

    updateStatus();

    if (!gameOver_ && engine_.getState().currentTurn == Color::Black) {
        startBotMove();
    }
}

void Gui::startBotMove() {
    botThinking_ = true;
    statusMessage_ = "Computer is thinking...";

    // Run bot in a separate thread
    botFuture_ = std::async(std::launch::async, [this]() -> Move {
        return bot_.findBestMove(engine_, Color::Black);
    });
}

void Gui::checkBotMove() {
    if (!botFuture_.valid()) return;
    auto status = botFuture_.wait_for(std::chrono::milliseconds(0));
    if (status == std::future_status::ready) {
        Move m = botFuture_.get();
        botThinking_ = false;
        if (m.fromRow >= 0) { // valid move
            engine_.applyMove(m);
            lastMove_ = m;
        }
        updateStatus();
    }
}

void Gui::updateStatus() {
    Color turn = engine_.getState().currentTurn;
    if (engine_.isCheckmate()) {
        gameOver_ = true;
        Color winner = engine_.getWinner();
        statusMessage_ = (winner == Color::White) ? "Checkmate! White wins!" : "Checkmate! Black wins!";
    } else if (engine_.isStalemate()) {
        gameOver_ = true;
        statusMessage_ = "Stalemate! It's a draw!";
    } else if (engine_.isInCheck(turn)) {
        statusMessage_ = (turn == Color::White) ? "White is in check!" : "Black is in check!";
    } else {
        statusMessage_ = (turn == Color::White) ? "Your turn (White)" : "Black to move";
    }
}

// ============================================================
// Promotion dialog
// ============================================================

void Gui::handlePromoClick(int x, int y) {
    static const PieceType choices[] = {
        PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight
    };
    int dialogW = 4 * CELL_SIZE + 20;
    int dialogH = CELL_SIZE + 60;
    int dialogX = (WINDOW_W - dialogW) / 2;
    int dialogY = (WINDOW_H - dialogH) / 2;
    // Pieces are drawn at dialogY + 30
    int piecesY = dialogY + 30;
    int piecesX = dialogX + 10;
    if (y >= piecesY && y < piecesY + CELL_SIZE) {
        int idx = (x - piecesX) / CELL_SIZE;
        if (idx >= 0 && idx < 4) {
            showPromoDialog_ = false;
            Move promoMove = pendingPromoMove_;
            promoMove.promotionPiece = choices[idx];
            executeMove(promoMove);
        }
    }
}

// ============================================================
// Drawing
// ============================================================

void Gui::draw() {
    drawBoard();
    drawHighlights();
    drawPieces();
    if (isDragging_ && hasSelection_) {
        drawDraggedPiece();
    }
    drawStatus();
    if (showPromoDialog_) {
        drawPromoDialog();
    }
}

void Gui::drawBoard() {
    sf::RectangleShape cell(sf::Vector2f(CELL_SIZE, CELL_SIZE));
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            cell.setPosition(c * CELL_SIZE, r * CELL_SIZE);
            cell.setFillColor((r + c) % 2 == 0 ? lightSquareColor_ : darkSquareColor_);
            window_.draw(cell);
        }
    }

    // Draw rank/file labels
    if (font_.getInfo().family != "") {
        for (int i = 0; i < 8; ++i) {
            // Rank numbers (8 at top, 1 at bottom)
            sf::Text rankLabel;
            rankLabel.setFont(font_);
            rankLabel.setCharacterSize(12);
            rankLabel.setString(std::to_string(8 - i));
            rankLabel.setFillColor(i % 2 == 0 ? darkSquareColor_ : lightSquareColor_);
            rankLabel.setPosition(2, i * CELL_SIZE + 2);
            window_.draw(rankLabel);

            // File letters (a-h)
            sf::Text fileLabel;
            fileLabel.setFont(font_);
            fileLabel.setCharacterSize(12);
            fileLabel.setString(std::string(1, 'a' + i));
            fileLabel.setFillColor((i + 7) % 2 == 0 ? darkSquareColor_ : lightSquareColor_);
            fileLabel.setPosition(i * CELL_SIZE + CELL_SIZE - 14, CELL_SIZE * 8 - 16);
            window_.draw(fileLabel);
        }
    }
}

void Gui::drawHighlights() {
    sf::RectangleShape highlight(sf::Vector2f(CELL_SIZE, CELL_SIZE));

    // Last move highlight
    if (lastMove_) {
        highlight.setFillColor(lastMoveColor_);
        highlight.setPosition(lastMove_->fromCol * CELL_SIZE, lastMove_->fromRow * CELL_SIZE);
        window_.draw(highlight);
        highlight.setPosition(lastMove_->toCol * CELL_SIZE, lastMove_->toRow * CELL_SIZE);
        window_.draw(highlight);
    }

    // King in check highlight
    if (engine_.isInCheck(engine_.getState().currentTurn)) {
        auto [kr, kc] = engine_.findKing(engine_.getState().currentTurn);
        if (kr >= 0) {
            highlight.setFillColor(checkColor_);
            highlight.setPosition(kc * CELL_SIZE, kr * CELL_SIZE);
            window_.draw(highlight);
        }
    }

    // Selected piece highlight
    if (hasSelection_) {
        highlight.setFillColor(selectedColor_);
        highlight.setPosition(selectedCol_ * CELL_SIZE, selectedRow_ * CELL_SIZE);
        window_.draw(highlight);

        // Legal move dots
        sf::CircleShape dot(10.f);
        dot.setFillColor(legalMoveColor_);
        for (const auto& m : legalMovesForSelected_) {
            // If target is occupied by enemy, draw a ring
            const Piece& target = engine_.getState().board[m.toRow][m.toCol];
            if (!target.isEmpty()) {
                // Draw corner indicators for capture squares
                sf::RectangleShape capture(sf::Vector2f(CELL_SIZE, CELL_SIZE));
                capture.setFillColor({200, 80, 80, 140});
                capture.setPosition(m.toCol * CELL_SIZE, m.toRow * CELL_SIZE);
                window_.draw(capture);
            } else {
                dot.setPosition(
                    m.toCol * CELL_SIZE + (CELL_SIZE - 20.f) / 2.f,
                    m.toRow * CELL_SIZE + (CELL_SIZE - 20.f) / 2.f
                );
                window_.draw(dot);
            }
        }
    }
}

// Unicode chess symbols
sf::String Gui::pieceToUnicode(const Piece& piece) {
    if (piece.isEmpty()) return "";
    static const wchar_t symbols[2][7] = {
        // None, Pawn,    Knight, Bishop, Rook,   Queen,  King
        {L'\0', L'\u2659', L'\u2658', L'\u2657', L'\u2656', L'\u2655', L'\u2654'}, // White
        {L'\0', L'\u265F', L'\u265E', L'\u265D', L'\u265C', L'\u265B', L'\u265A'}  // Black
    };
    int ci = (piece.color == Color::White) ? 0 : 1;
    int pi = static_cast<int>(piece.type);
    if (pi <= 0 || pi > 6) return "";
    wchar_t ch = symbols[ci][pi];
    if (ch == L'\0') return "";
    return sf::String(ch);
}

void Gui::drawPiece(const Piece& piece, float x, float y, float size, bool transparent) {
    if (piece.isEmpty()) return;

    // Try Unicode text rendering first
    if (font_.getInfo().family != "") {
        sf::Text text;
        text.setFont(font_);
        text.setString(pieceToUnicode(piece));
        text.setCharacterSize(static_cast<unsigned int>(size * 0.75f));

        sf::Color fillColor = (piece.color == Color::White) ?
            sf::Color(255, 255, 255) : sf::Color(20, 20, 20);
        sf::Color outlineColor = (piece.color == Color::White) ?
            sf::Color(40, 40, 40) : sf::Color(240, 240, 240);

        if (transparent) {
            fillColor.a = 160;
            outlineColor.a = 160;
        }

        text.setFillColor(fillColor);
        text.setOutlineColor(outlineColor);
        text.setOutlineThickness(1.5f);

        // Center text in cell
        sf::FloatRect bounds = text.getLocalBounds();
        text.setPosition(
            x + (size - bounds.width) / 2.f - bounds.left,
            y + (size - bounds.height) / 2.f - bounds.top
        );
        window_.draw(text);
        return;
    }

    // Fallback: draw colored circles/shapes
    float margin = size * 0.1f;
    float pieceSize = size - 2 * margin;

    sf::CircleShape circle(pieceSize / 2.f);
    sf::Color baseColor = (piece.color == Color::White) ?
        sf::Color(240, 240, 240) : sf::Color(30, 30, 30);
    sf::Color outlineClr = (piece.color == Color::White) ?
        sf::Color(40, 40, 40) : sf::Color(200, 200, 200);

    if (transparent) baseColor.a = 160;

    circle.setFillColor(baseColor);
    circle.setOutlineColor(outlineClr);
    circle.setOutlineThickness(2.f);
    circle.setPosition(x + margin, y + margin);
    window_.draw(circle);

    // Draw a letter for piece type
    if (font_.getInfo().family != "") {
        static const char letters[] = " PNBRQK";
        sf::Text label;
        label.setFont(font_);
        label.setString(std::string(1, letters[static_cast<int>(piece.type)]));
        label.setCharacterSize(static_cast<unsigned int>(pieceSize * 0.5f));
        label.setFillColor(outlineClr);
        sf::FloatRect b = label.getLocalBounds();
        label.setPosition(x + (size - b.width) / 2.f - b.left,
                          y + (size - b.height) / 2.f - b.top);
        window_.draw(label);
    }
}

void Gui::drawPieces() {
    for (int r = 0; r < 8; ++r) {
        for (int c = 0; c < 8; ++c) {
            const Piece& p = engine_.getState().board[r][c];
            if (p.isEmpty()) continue;
            // Skip dragged piece at its original position
            if (isDragging_ && hasSelection_ && r == selectedRow_ && c == selectedCol_) continue;
            drawPiece(p, static_cast<float>(c * CELL_SIZE), static_cast<float>(r * CELL_SIZE), CELL_SIZE);
        }
    }
}

void Gui::drawDraggedPiece() {
    const Piece& p = engine_.getState().board[selectedRow_][selectedCol_];
    drawPiece(p, dragPos_.x - dragOffset_.x, dragPos_.y - dragOffset_.y, CELL_SIZE);
}

void Gui::drawStatus() {
    // Status bar at the bottom
    sf::RectangleShape bar(sf::Vector2f(WINDOW_W, 60));
    bar.setPosition(0, CELL_SIZE * 8);
    bar.setFillColor(sf::Color(50, 50, 50));
    window_.draw(bar);

    if (font_.getInfo().family != "") {
        sf::Text status;
        status.setFont(font_);
        status.setString(statusMessage_);
        status.setCharacterSize(18);
        status.setFillColor(sf::Color::White);
        sf::FloatRect b = status.getLocalBounds();
        status.setPosition((WINDOW_W - b.width) / 2.f, CELL_SIZE * 8 + (60 - b.height) / 2.f - 4.f);
        window_.draw(status);
    }

    // Turn indicator circles
    sf::CircleShape indicator(12.f);
    indicator.setOutlineThickness(2.f);
    indicator.setOutlineColor(sf::Color(200, 200, 200));
    Color turn = engine_.getState().currentTurn;
    indicator.setFillColor(turn == Color::White ? sf::Color::White : sf::Color(30, 30, 30));
    indicator.setPosition(10.f, CELL_SIZE * 8 + 18.f);
    window_.draw(indicator);
}

void Gui::drawPromoDialog() {
    // Semi-transparent overlay
    sf::RectangleShape overlay(sf::Vector2f(WINDOW_W, WINDOW_H));
    overlay.setFillColor(sf::Color(0, 0, 0, 150));
    window_.draw(overlay);

    // Dialog box
    int dialogW = 4 * CELL_SIZE + 20;
    int dialogH = CELL_SIZE + 60;
    int dialogX = (WINDOW_W - dialogW) / 2;
    int dialogY = (WINDOW_H - dialogH) / 2;

    sf::RectangleShape dialog(sf::Vector2f(dialogW, dialogH));
    dialog.setPosition(dialogX, dialogY);
    dialog.setFillColor(sf::Color(60, 60, 60));
    dialog.setOutlineColor(sf::Color(200, 200, 200));
    dialog.setOutlineThickness(2.f);
    window_.draw(dialog);

    // Title
    if (font_.getInfo().family != "") {
        sf::Text title;
        title.setFont(font_);
        title.setString("Choose promotion piece:");
        title.setCharacterSize(16);
        title.setFillColor(sf::Color::White);
        sf::FloatRect b = title.getLocalBounds();
        title.setPosition(dialogX + (dialogW - b.width) / 2.f, dialogY + 5.f);
        window_.draw(title);
    }

    // Piece choices: Queen, Rook, Bishop, Knight
    static const PieceType choices[] = {
        PieceType::Queen, PieceType::Rook, PieceType::Bishop, PieceType::Knight
    };
    // Human (White) promotion pieces
    for (int i = 0; i < 4; ++i) {
        float px = dialogX + 10.f + i * CELL_SIZE;
        float py = dialogY + 30.f;

        sf::RectangleShape cell(sf::Vector2f(CELL_SIZE - 4, CELL_SIZE - 4));
        cell.setPosition(px + 2, py + 2);
        cell.setFillColor((i % 2 == 0) ? sf::Color(200, 180, 150) : sf::Color(150, 110, 70));
        window_.draw(cell);

        Piece promoP{choices[i], Color::White};
        drawPiece(promoP, px, py, CELL_SIZE);
    }
}
