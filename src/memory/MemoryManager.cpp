#include "MemoryManager.hpp"

MemoryManager::MemoryManager(size_t mainMemorySize, size_t secondaryMemorySize) {
    mainMemory = std::make_unique<MAIN_MEMORY>(mainMemorySize);
    secondaryMemory = std::make_unique<SECONDARY_MEMORY>(secondaryMemorySize);
    L1_cache = std::make_unique<Cache>(); // Inicializa a cache
    mainMemoryLimit = mainMemorySize;
    timestamp = 0;
}

uint32_t MemoryManager::read(uint32_t address, PCB& process) {
    timestamp++;
    process.mem_accesses_total.fetch_add(1);
    process.mem_reads.fetch_add(1);

    // 1. Tenta ler da Cache
    size_t cache_data = L1_cache->get(address);
    if (cache_data != CACHE_MISS) {
        process.cache_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.cache);
        return cache_data;
    }

    // 2. Cache Miss: busca na memória correta (RAM ou Disco)
    uint32_t data_from_mem;
    if (address < mainMemoryLimit) {
        // Acesso à Memória Principal (RAM)
        process.primary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.primary);
        data_from_mem = mainMemory->ReadMem(address);
    } else {
        // Acesso à Memória Secundária (Disco)
        process.secondary_mem_accesses.fetch_add(1);
        process.memory_cycles.fetch_add(process.memWeights.secondary);
        uint32_t secondaryAddress = address - mainMemoryLimit;
        data_from_mem = secondaryMemory->ReadMem(secondaryAddress);
    }

    // 3. Após a busca, armazena o dado na cache
    L1_cache->put(address, data_from_mem, timestamp);
    
    return data_from_mem;
}

void MemoryManager::write(uint32_t address, uint32_t data, PCB& process) {
    timestamp++;
    process.mem_accesses_total.fetch_add(1);
    process.mem_writes.fetch_add(1);

    // Política de escrita: Write-Back (conforme Pratica0.pdf)
    // A escrita é feita diretamente na cache e marcada como "dirty".
    L1_cache->update(address, data);
    process.cache_mem_accesses.fetch_add(1);
    process.memory_cycles.fetch_add(process.memWeights.cache);

    // A escrita na memória principal/secundária só ocorrerá quando
    // a linha da cache for substituída (eviction). Essa lógica deve ser
    // implementada dentro de Cache::put() ou CachePolicy::erase().
    // Quando uma linha "dirty" é removida, o `MemoryManager` deve ser notificado
    // para escrever o dado de volta na memória principal/secundária.
}