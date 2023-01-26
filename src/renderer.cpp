#include "renderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <array>
#include <fmt/format.h>

#include "assets.hpp"

namespace xt {
    BoardRenderer::BoardRenderer(Board &board) : mBoard(board) {
        mTexture.create(BOARD_SIZE, BOARD_SIZE);
        mBackground.create(mTexture.getSize().x, mTexture.getSize().y);
        mPieces.create(mTexture.getSize().x, mTexture.getSize().y);

        mPieceTexture.loadFromMemory(PIECES_PNG, PIECES_PNG_LEN);
        mFont.loadFromMemory(COMFORTAA_TTF, COMFORTAA_TTF_LEN);

        for (int team = 0; team < Team::MAX; team++) {
            for (int piece = 0; piece < Piece::MAX; piece++) {
                mSprites[team][piece].setTexture(mPieceTexture);
                mSprites[team][piece].setTextureRect(
                    sf::IntRect(piece * PIECE_SIZE, team * PIECE_SIZE, PIECE_SIZE, PIECE_SIZE));
            }
        }

        UpdateTitle();
    }

    void BoardRenderer::ProcessEvent(sf::RenderTarget &target, sf::Event &event) {
        switch (event.type) {
        case sf::Event::MouseButtonPressed:
            if (event.mouseButton.button == sf::Mouse::Left) {
                if (mBoard.GetPromoting()) {
                    if (mPromotion != Piece::MAX) {
                        mBoard.Promote(mPromotion);
                        mPromotion = Piece::MAX;

                        UpdateTitle();
                    }
                } else if (auto pos = ScreenToCoords(target)) {
                    if (!mBoard[*pos].IsEmpty())
                        mSelected = *pos;
                }
            }
            break;
        case sf::Event::MouseButtonReleased:
            if (event.mouseButton.button == sf::Mouse::Left) {
                if (mBoard.IsValid(mSelected))
                    if (auto pos = ScreenToCoords(target))
                        if (mBoard.TryMove(mSelected, *pos))
                            UpdateTitle();

                mSelected = INVALID_POS;
            }

            break;
        case sf::Event::MouseMoved:
            mMouse = {event.mouseMove.x, event.mouseMove.y};
            break;
        default:
            return;
        }
    }

    void BoardRenderer::Render(sf::RenderTarget &target) {
        mTexture.clear(sf::Color::Black);
        mBackground.clear(sf::Color::Black);
        mPieces.clear(sf::Color::Transparent);

        for (Int y = 0; y < Board::SIZE; y++) {
            for (Int x = 0; x < Board::SIZE; x++) {
                static const sf::Color LIGHT{227, 214, 182};
                static const sf::Color DARK{169, 105, 61};

                const sf::Vector2f world(x * PIECE_SIZE, y * PIECE_SIZE);
                const Vector       position(x, y);
                const bool lightSquare = (y % 2 != 0 || x % 2 != 1) && (y % 2 != 1 || x % 2 != 0);

                sf::RectangleShape square(sf::Vector2f(PIECE_SIZE, PIECE_SIZE));

                square.setPosition(world);
                square.setFillColor(lightSquare ? LIGHT : DARK);

                // TODO: Kinda lazy
                if (mBoard[{x, y}].OpposingTeam(mBoard.GetTurn()) &&
                    mBoard.IsValidMove(position, mBoard.GetKing(mBoard.GetTurn()))) {
                    square.setOutlineColor({square.getFillColor().r,
                                            static_cast<sf::Uint8>(square.getFillColor().g / 4),
                                            static_cast<sf::Uint8>(square.getFillColor().b / 4),
                                            255});
                    square.setOutlineThickness(-4.f);
                }

                if (mBoard.IsValid(mSelected) && mBoard.IsValidMove(mSelected, position))
                    square.setFillColor({square.getFillColor().r,
                                         square.getFillColor().g,
                                         square.getFillColor().b,
                                         180});

                mBackground.draw(square);

                if (x == 0) {
                    sf::Text text(fmt::format("{}", Board::SIZE - y), mFont, 12);
                    text.setPosition(world + sf::Vector2f{2.f, 2.f});
                    text.setFillColor(lightSquare ? DARK : LIGHT);

                    mBackground.draw(text);
                }

                if (y == Board::SIZE - 1) {
                    sf::Text text(fmt::format("{}", static_cast<char>('A' + x)), mFont, 12);
                    text.setPosition(
                        world + sf::Vector2f{PIECE_SIZE - text.getLocalBounds().width - 4.f, 2.f});
                    text.setFillColor(lightSquare ? DARK : LIGHT);

                    mBackground.draw(text);
                }

                if (const auto &piece = mBoard[{x, y}]; !piece.IsEmpty()) {
                    auto &sprite = mSprites[piece.team][piece.type];
                    sprite.setPosition(sf::Vector2f(x * PIECE_SIZE, y * PIECE_SIZE));
                    if (x == mSelected.x && y == mSelected.y)
                        continue;

                    mPieces.draw(sprite);
                }
            }
        }

        if (mBoard.IsValid(mSelected)) {
            const auto &piece  = mBoard[mSelected];
            auto       &sprite = mSprites[piece.team][piece.type];
            sprite.setPosition(GetMousePosition(target) -
                               sf::Vector2f(PIECE_SIZE / 2.f, PIECE_SIZE / 2.f));
            mPieces.draw(sprite);
        }

        mBackground.display();
        mPieces.display();

        sf::Sprite background(mBackground.getTexture());
        sf::Sprite pieces(mPieces.getTexture());

        mTexture.draw(background);
        mTexture.draw(pieces);

        if (mBoard.GetPromoting())
            RenderPromotionDialog(mTexture);

        mTexture.display();

        sf::Sprite board(mTexture.getTexture());
        board.setPosition(mPosition);

        target.draw(board);
    }

    void BoardRenderer::RenderPromotionDialog(sf::RenderTarget &target) {
        sf::Text header("Promote", mFont, 15);

        static const std::array promotions = {
            Piece::QUEEN, Piece::KNIGHT, Piece::ROOK, Piece::BISHOP};
        static const auto PADDING = 5.f;

        const auto headerY = header.getLocalBounds().height + PADDING * 2;

        sf::RectangleShape background(
            sf::Vector2f{(PIECE_SIZE + PADDING * 2),
                         headerY + (PIECE_SIZE + PADDING) * promotions.size() + PADDING});
        background.setFillColor(sf::Color{70, 70, 70});
        background.setPosition(sf::Vector2f{mBackground.getSize()} / 2.f -
                               background.getSize() / 2.f);

        header.setPosition(
            sf::Vector2f{(background.getPosition().x + background.getSize().x / 2.f) -
                             header.getLocalBounds().width / 2.f,
                         background.getPosition().y});

        target.draw(background);
        target.draw(header);

        mPromotion = Piece::MAX;

        float y = headerY + PADDING;
        for (const auto &piece : promotions) {
            auto &sprite = mSprites[mBoard.GetPromoting()->team][piece];
            sprite.setPosition(
                sf::Vector2f{background.getPosition().x + PADDING, background.getPosition().y + y});

            if (sprite.getGlobalBounds().contains(GetMousePosition(target))) {
                sf::RectangleShape background(sf::Vector2f{PIECE_SIZE, PIECE_SIZE});
                background.setPosition(sprite.getPosition());
                background.setFillColor(sf::Color{0, 128, 255, 180});
                target.draw(background);

                mPromotion = piece;
            }

            target.draw(sprite);
            y += PIECE_SIZE + PADDING;
        }
    }

    std::string BoardRenderer::GetTitle() const {
        return mTitle;
    }

    void BoardRenderer::UpdateTitle() {
        switch (mBoard.GetStatus()) {
        case Board::ACTIVE:
            mTitle = mBoard.GetTurn() == Team::BLACK ? "Black" : "White";
            break;
        case Board::CHECKMATE:
            mTitle = "Checkmate!";
            break;
        case Board::STALEMATE:
            mTitle = "Stalemate!";
            break;
        }
    }

    void BoardRenderer::SetPosition(const sf::Vector2f &position) {
        mPosition = position;
    }

    std::optional<Vector> BoardRenderer::ScreenToCoords(sf::RenderTarget &target) const {
        const auto mouse = GetMousePosition(target);
        const auto result =
            Vector{static_cast<Int>(mouse.x / PIECE_SIZE), static_cast<Int>(mouse.y / PIECE_SIZE)};
        if (!mBoard.IsValid(result))
            return {};
        return result;
    }

    sf::Vector2f BoardRenderer::GetMousePosition(sf::RenderTarget &target) const {
        return target.mapPixelToCoords(mMouse) - mPosition;
    }
} // namespace xt