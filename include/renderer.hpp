#pragma once

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Sprite.hpp>
#include <SFML/Graphics/Texture.hpp>
#include <SFML/Graphics/Font.hpp>

#include <optional>

#include "board.hpp"

namespace zc
{
    class BoardRenderer
    {
    public:
        static constexpr const int PIECE_SIZE = 60;

    public:
        BoardRenderer(Board &board);

        std::string GetTitle() const;

        void ProcessEvent(sf::RenderWindow &window, sf::Event &event);
        void Render(sf::RenderWindow &window);
        void UpdateTitle();

    private:
        void RenderPromotionDialog(sf::RenderWindow &window);

        std::optional<Vector> ScreenToCoords(sf::RenderWindow &window) const;

    private:
        sf::Sprite   mSprites[Team::MAX][Piece::MAX];
        sf::Texture  mPieces;
        sf::Font     mFont;
        std::string  mTitle{"White"};

        Board       &mBoard;
        Vector     mSelected{INVALID_POS};
        Piece::Type  mPromotion{Piece::MAX};
        
    };
}