#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <fmt/format.h>

#include "renderer.hpp"

int main(int argc, char **)
{
    srand(time(nullptr));
    
    sf::RenderWindow window(sf::VideoMode(zc::BoardRenderer::PIECE_SIZE * 8, zc::BoardRenderer::PIECE_SIZE * 8), "", sf::Style::Close);
    sf::Clock clock;
    sf::Time last, update;

    zc::Board board;
    zc::BoardRenderer renderer(board);
    while (window.isOpen())
    {
        const auto now = clock.getElapsedTime();
        const auto fps = 1.f / (now - last).asSeconds();
        last = now;

        sf::Event event;
        while (window.pollEvent(event))
        {
            switch (event.type)
            {
            case sf::Event::Closed:
                window.close();
                break;
            default:
                renderer.ProcessEvent(window, event);
                break;
            }
        }

        window.clear(sf::Color::Black);
        renderer.Render(window);
        window.display();

        static std::string status;
        if ((clock.getElapsedTime() - update).asSeconds() > 1.f)
        {
            update = now;
            window.setTitle(fmt::format("{} ({:.0f} FPS)", status, fps));
        }

        if (status != renderer.GetTitle())
        {
            update = now;
            status = renderer.GetTitle();
            window.setTitle(fmt::format("{} ({:.0f} FPS)", status, fps));
        }

        if (argc > 1)
        {
            static int player = rand() % 2 == 0 ? zc::Board::PLAYERB : zc::Board::PLAYERW;
            
            if (const auto *piece = board.GetPromoting())
                if (piece->team == player)
                    board.Promote(zc::Piece::QUEEN);

            if (board.GetTurn() == player)
            {
                const auto moves = board.GetValidMoves(board.GetTurn());
                if (!moves.empty())
                {
                    const auto move = moves[rand() % moves.size()];
                    if (board.TryMove(move.first, move.second))
                        renderer.UpdateTitle();
                }
            }
        }
    }

    return 0;
}