#pragma once

#include <SFML/Graphics/Font.hpp>
#include <SFML/Graphics/RenderTexture.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <optional>

#include "board.hpp"

namespace xt {
    class BoardRenderer {
    public:
        static constexpr const int PIECE_SIZE = 60;
        static constexpr const int BOARD_SIZE = PIECE_SIZE * Board::SIZE;

    public:
        BoardRenderer(Board &board);

        std::string GetTitle() const;

        void ProcessEvent(sf::RenderTarget &target, sf::Event &event);
        void Render(sf::RenderTarget &target);
        void UpdateTitle();
        void SetPosition(const sf::Vector2f &position);

    private:
        void RenderPromotionDialog(sf::RenderTarget &target);

        std::optional<Vector> ScreenToCoords(sf::RenderTarget &target) const;
        sf::Vector2f          GetMousePosition(sf::RenderTarget &target) const;

    private:
        sf::RenderTexture mTexture;
        sf::RenderTexture mBackground;
        sf::RenderTexture mPieces;

        sf::Vector2f mPosition;
        sf::Vector2i mMouse;
        sf::Sprite   mSprites[Team::MAX][Piece::MAX];
        sf::Texture  mPieceTexture;
        sf::Font     mFont;
        std::string  mTitle;

        Board      &mBoard;
        Vector      mSelected{INVALID_POS};
        Piece::Type mPromotion{Piece::MAX};
    };
} // namespace xt