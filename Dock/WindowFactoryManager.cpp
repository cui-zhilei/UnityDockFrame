#include "WindowFactoryManager.h"
namespace dock {

WindowFactoryManager* WindowFactoryManager::getInstance()
{
    static WindowFactoryManager instance;
    return &instance;
}

WindowFactoryManager::WindowFactoryManager()
{
}

WindowFactoryManager::~WindowFactoryManager()
{
    for (size_t i = 0; i < _needDeletefactorys.size(); i++)
    {
        delete _needDeletefactorys[i];
    }
}

void WindowFactoryManager::registerFactory(uint typeId, WindowFactory *fac, bool needDelete)
{
    _factorys.insert(std::make_pair(typeId, fac));
    if (needDelete)
    {
        _needDeletefactorys.push_back(fac);
    }
}

WindowFactory * WindowFactoryManager::getFactory(uint typeId)
{
    if (_factorys.empty())
    {
        return nullptr;
    }

    if (typeId == 0)
    {
        return _factorys.begin()->second;
    }
    auto it = _factorys.find(typeId);
    if (it != _factorys.end())
    {
        return _factorys.at(typeId);
    }
    return nullptr;
}


}

