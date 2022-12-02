
namespace vkroots::helpers {

  template <typename Func>
  inline void delimitStringView(std::string_view view, std::string_view delim, Func func) {
      size_t pos = 0;
      while ((pos = view.find(delim)) != std::string_view::npos) {
          std::string_view token = view.substr(0, pos);
          if (!func(token))
              return;
          view = view.substr(pos + 1);
      }
      func(view);
  }

  template <typename T, typename ArrType, typename Op>
  inline VkResult vkarray(ArrType& arr, uint32_t *pCount, T* pOut, Op func) {
      const uint32_t count = uint32_t(arr.size());

      if (!pOut) {
          *pCount = count;
          return VK_SUCCESS;
      }

      const uint32_t outCount = std::min(*pCount, count);
      for (uint32_t i = 0; i < outCount; i++)
          func(pOut[i], arr[i]);

      *pCount = outCount;
      return count != outCount
          ? VK_INCOMPLETE
          : VK_SUCCESS;
  }

  template <typename Key, typename Data>
  class SynchronizedMapObject {
  public:
    using MapKey = Key;
    using MapData = Data;

    static std::optional<SynchronizedMapObject> get(const Key& key) {
      std::unique_lock lock{ s_mutex };
      auto iter = s_map.find(key);
      if (iter == s_map.end())
        return std::nullopt;
      return SynchronizedMapObject{ iter->second, s_mutex, std::adopt_lock };
    }

    static SynchronizedMapObject create(const Key& key, Data data) {
      std::unique_lock lock{ s_mutex };
      auto val = s_map.insert(std::make_pair(key, std::move(data)));
      return SynchronizedMapObject{ val.first->second, s_mutex, std::adopt_lock };
    }

    static bool remove(const Key& key) {
      std::unique_lock lock{ s_mutex };
      auto iter = s_map.find(key);
      if (iter == s_map.end())
        return false;
      s_map.erase(iter);
      return true;
    }

    Data* get() {
      return &m_data;
    }

    const Data* get() const {
      return &m_data;
    }

    Data* operator->() {
      return get();
    }

    const Data* operator->() const {
      return get();
    }
  private:
    SynchronizedMapObject(Data& data, std::mutex& mutex)
        : m_data{ data }, m_lock{ mutex } {}

    SynchronizedMapObject(Data& data, std::mutex& mutex, std::adopt_lock_t adopt) noexcept
        : m_data{ data }, m_lock{ mutex, adopt } {}

    Data &m_data;
    std::unique_lock<std::mutex> m_lock;

    static std::mutex s_mutex;
    static std::unordered_map<Key, Data> s_map;
  };

#define VKROOTS_DEFINE_SYNCHRONIZED_MAP_TYPE(name, key) \
  using name = ::vkroots::helpers::SynchronizedMapObject<key, name##Data>;

#define VKROOTS_IMPLEMENT_SYNCHRONIZED_MAP_TYPE(x) \
  template <> std::mutex x::s_mutex = {}; \
  template <> std::unordered_map<x::MapKey, x::MapData> x::s_map = {};

}
