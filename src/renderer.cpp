#include "renderer.hpp"

#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/Text.hpp>
#include <SFML/Window/Event.hpp>

#include <fmt/format.h>
#include <array>

#include "assets.hpp"

namespace zc
{
    BoardRenderer::BoardRenderer(Board &board)
        : mBoard(board)
    {
        mPieces.loadFromMemory(PIECES_PNG, PIECES_PNG_LEN);
        mFont.loadFromMemory(COMFORTAA_TTF, COMFORTAA_TTF_LEN);

        for (int team = 0; team < 2; team++)
        {
            for (int piece = 0; piece < Piece::MAX; piece++)
            {
                mSprites[team][piece].setTexture(mPieces);
                mSprites[team][piece].setTextureRect(sf::IntRect(
                    piece * PIECE_SIZE,
                    team * PIECE_SIZE,
                    PIECE_SIZE,
                    PIECE_SIZE));
            }
        }
    }

    void BoardRenderer::ProcessEvent(sf::RenderWindow &window, sf::Event &event)
    {
        switch (event.type)
        {
        case sf::Event::MouseButtonPressed:
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                if (mBoard.GetPromoting())
                {
                    if (mPromotion != Piece::MAX)
                    {
                        mBoard.Promote(mPromotion);
                        mPromotion = Piece::MAX;

                        UpdateTitle();
                    }
                } else if (auto pos = ScreenToCoords(window))
                {
                    if (!mBoard[*pos].IsEmpty())
                        mSelected = *pos;
                }
            }
            break;
        case sf::Event::MouseButtonReleased:
            if (event.mouseButton.button == sf::Mouse::Left)
            {
                if (mBoard.IsValid(mSelected))
                    if (auto pos = ScreenToCoords(window))
                        if (mBoard.TryMove(mSelected, *pos))
                            UpdateTitle();

                mSelected = INVALID_POS;
            }

            break;
        default:
            return;
        }
    }

    void BoardRenderer::Render(sf::RenderWindow &window)
    {
        for (Int y = 0; y < Board::SIZE; y++)
        {
            for (Int x = 0; x < Board::SIZE; x++)
            {
                static const sf::Color LIGHT{227, 214, 182};
                static const sf::Color DARK{169, 105, 61};

                const sf::Vector2f world(x * PIECE_SIZE, y * PIECE_SIZE);
                const Vector position(x, y);

                const bool lightSquare = (y % 2 != 0 || x % 2 != 1) && (y % 2 != 1 || x % 2 != 0);

                sf::RectangleShape rect(sf::Vector2f(PIECE_SIZE, PIECE_SIZE));

                rect.setPosition(world);
                rect.setFillColor(lightSquare ? LIGHT : DARK);

                // TODO: Kinda lazy
                if (mBoard[{x, y}].OpposingTeam(mBoard.GetTurn()) && mBoard.IsValidMove(position, mBoard.GetKing(mBoard.GetTurn())))
                    rect.setFillColor({
                        rect.getFillColor().r,
                        static_cast<sf::Uint8>(rect.getFillColor().g / 4),
                        static_cast<sf::Uint8>(rect.getFillColor().b / 4),
                        255});

                if (mBoard.IsValid(mSelected) && mBoard.IsValidMove(mSelected, position))
                    rect.setFillColor({rect.getFillColor().r, rect.getFillColor().g, rect.getFillColor().b, 180});

                window.draw(rect);

                if (x == 0)
                {
                    sf::Text text(fmt::format("{}", Board::SIZE - y), mFont, 12);
                    text.setPosition(world + sf::Vector2f{2.f, 2.f});
                    text.setFillColor(lightSquare ? DARK : LIGHT);

                    window.draw(text);
                }

                if (y == Board::SIZE - 1)
                {
                    sf::Text text(fmt::format("{}", static_cast<char>('A' + x)), mFont, 12);
                    text.setPosition(world + sf::Vector2f{PIECE_SIZE - text.getLocalBounds().width - 4.f, 2.f});
                    text.setFillColor(lightSquare ? DARK : LIGHT);

                    window.draw(text);
                }
            }
        }

        for (Int y = 0; y < Board::SIZE; y++)
        {
            for (Int x = 0; x < Board::SIZE; x++)
            {
                if (const auto &piece = mBoard[{x, y}]; !piece.IsEmpty())
                {
                    auto &sprite = mSprites[piece.team][piece.type];
                    sprite.setPosition(sf::Vector2f(x * PIECE_SIZE, y * PIECE_SIZE));
                    if (x == mSelected.x && y == mSelected.y)
                        continue;

                    window.draw(sprite);
                }
            }
        }

        if (mBoard.IsValid(mSelected))
        {
            const auto &piece = mBoard[mSelected];
            auto &sprite = mSprites[piece.team][piece.type];
            sprite.setPosition(window.mapPixelToCoords(sf::Mouse::getPosition(window))
                - sf::Vector2f(PIECE_SIZE / 2.f, PIECE_SIZE / 2.f));
            window.draw(sprite);
        }

        if (mBoard.GetPromoting())
            RenderPromotionDialog(window);
    }

    void BoardRenderer::RenderPromotionDialog(sf::RenderWindow &window)
    {
        sf::Text text("Promote", mFont, 15);

        static const std::array promotions = {Piece::QUEEN, Piece::KNIGHT, Piece::ROOK, Piece::BISHOP};
        static const auto PADDING = 5.f;

        const auto header = text.getLocalBounds().height + PADDING * 2;

        sf::RectangleShape shape(sf::Vector2f{(PIECE_SIZE + PADDING * 2), header + (PIECE_SIZE + PADDING) * promotions.size() + PADDING});
        shape.setFillColor(sf::Color{70, 70, 70});
        shape.setPosition(window.getView().getSize() / 2.f - shape.getSize() / 2.f);

        text.setPosition(sf::Vector2f{
            (shape.getPosition().x + shape.getSize().x / 2.f) - text.getLocalBounds().width / 2.f,
            shape.getPosition().y});

        window.draw(shape);
        window.draw(text);

        mPromotion = Piece::MAX;

        float y = header + PADDING;
        for (auto &piece : promotions)
        {
            auto &sprite = mSprites[mBoard.GetPromoting()->team][piece];
            sprite.setPosition(sf::Vector2f{shape.getPosition().x + PADDING, shape.getPosition().y + y});

            if (sprite.getGlobalBounds().contains(sf::Vector2f{sf::Mouse::getPosition(window)}))
            {
                sf::RectangleShape background(sf::Vector2f{PIECE_SIZE, PIECE_SIZE});
                background.setPosition(sprite.getPosition());
                background.setFillColor(sf::Color{0, 128, 255, 180});

                window.draw(background);

                mPromotion = piece;
            }

            window.draw(sprite);
            y += PIECE_SIZE + PADDING;
        }
    }

    std::string BoardRenderer::GetTitle() const
    {
        return mTitle;
    }

    void BoardRenderer::UpdateTitle()
    {
        switch (mBoard.GetStatus())
        {
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

    std::optional<Vector> BoardRenderer::ScreenToCoords(sf::RenderWindow &window) const
    {
        const auto pos    = sf::Mouse::getPosition(window);
        const auto result = Vector{static_cast<Int>(pos.x / PIECE_SIZE), static_cast<Int>(pos.y / PIECE_SIZE)};
        if (!mBoard.IsValid(result))
            return {};
        return result;
    }

}