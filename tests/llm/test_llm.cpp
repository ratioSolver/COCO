#include "coco.hpp"
#include "coco_type.hpp"
#include "coco_item.hpp"
#include "coco_db.hpp"
#ifdef LLM_PROVIDER_OLLAMA
#include "coco_ollama.hpp"
#elif defined(LLM_PROVIDER_HUGGINGFACE)
#include "coco_huggingface.hpp"
#endif

int main([[maybe_unused]] int argc, [[maybe_unused]] char *argv[])
{
    coco::coco_db db;
    coco::coco cc(db);

#ifdef LLM_PROVIDER_OLLAMA
    auto &llm = cc.add_module<coco::coco_ollama>(cc);
#elif defined(LLM_PROVIDER_HUGGINGFACE)
    auto &llm = cc.add_module<coco::coco_huggingface>(cc);
#endif
    cc.load_rules();

    while (true)
    {
        std::string user_input;
        std::getline(std::cin, user_input);
        if (user_input == "exit" || user_input == "quit")
            break;
        auto response = llm.understand(user_input);
        std::cout << response << std::endl;
    }

    return 0;
}
