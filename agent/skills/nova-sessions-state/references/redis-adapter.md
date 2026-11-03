# NovaCPP Distributed Redis State Adapter Reference

## `np::IStateStore` Architecture

NovaCPP abstracts reactive state persistence behind the `np::IStateStore` interface (`novacpp/state.hpp`):

```cpp
namespace np {
    class IStateStore {
    public:
        virtual ~IStateStore() = default;
        virtual std::string get(const std::string& sessionId, const std::string& stateKey) = 0;
        virtual void set(const std::string& sessionId, const std::string& stateKey, const std::string& val) = 0;
        virtual void remove(const std::string& sessionId) = 0;
        virtual void sweepExpired(std::chrono::seconds maxAge) = 0;
    };
}
```

---

## Zero-Dependency Native Redis RESP Adapter

Located in `novacpp/redis_state.hpp`:
- Implements the lightweight native RESP (REdis Serialization Protocol) client without third-party C++ libraries.
- Supports Redis standalone, clustered gateways, and password authentication (`redis://:password@host:port`).
- Automatic Failover: If Redis disconnects during network partitions, `RedisStateStore` falls back to in-memory caching to prevent server downtime.

