#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Window/Event.hpp>
#include <fmt/format.h>

#include <fstream>

#include "renderer.hpp"

int main(int argc, char **argv)
{
    srand(time(nullptr));

    std::string save, load;
    bool ai = false;
    for (int i = 0; i < argc; i++)
    {
        if (!strcmp(argv[i], "-s") && i + 1 < argc)
            save = argv[++i];
        if (!strcmp(argv[i], "-l") && i + 1 < argc)
            load = argv[++i];
        if (!strcmp(argv[i], "-a"))
            ai = true;
    }
    
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
            case sf::Event::KeyReleased:
                if (!event.key.control)
                    break;

                switch (event.key.code)
                {
                case sf::Keyboard::S:
                {
                    std::ofstream file(save, std::ios::binary);
                    if (file.good())
                    {
                        const auto data = board.Save();
                        file.write(reinterpret_cast<const char *>(data.data()), data.size());

                        fmt::print("Saved to '{}'!\n", save);
                    }
                } break;
                case sf::Keyboard::L:
                {
                    std::ifstream file(load, std::ios::binary | std::ios::ate);
                    if (file.good())
                    {
                        const auto size = file.tellg();
                        file.seekg(std::ios::beg);

                        std::vector<std::uint8_t> data(size, '\0');
                        file.read(reinterpret_cast<char *>(data.data()), data.size());

                        if (board.Load(data))
                        {
                            renderer.UpdateTitle();
                            fmt::print("Loaded from '{}'!\n", load);
                            break;
                        }
                    }

                    fmt::print("Load from '{}' failed!\n", load);
                } break;
                default:
                    break;
                }

                break;
            default:
                break;
            }

            renderer.ProcessEvent(window, event);
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

        if (ai)
        {
            static const auto player = rand() % zc::Team::MAX == 0 ? zc::Team::BLACK : zc::Team::WHITE;
            
            if (const auto *piece = board.GetPromoting())
                if (piece->team == player)
                    board.Promote(zc::Piece::QUEEN);

            if (board.GetTurn() == player)
            {
                const auto moves = board.GetValidMoves(board.GetTurn());
                if (!moves.empty())
                {
                    const auto &move = moves[rand() % moves.size()];
                    if (board.TryMove(move.first, move.second))
                        renderer.UpdateTitle();
                }
            }
        }
    }

    return 0;
}