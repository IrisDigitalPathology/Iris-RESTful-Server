//
//  IrisQueueV2.h
//  Iris
//
//  Created by Ryan Landvater on 12/10/23.
//

#ifndef IrisQueueV2_h
#define IrisQueueV2_h

namespace Iris {
namespace FIFO2 {
#if IRIS_DEBUG
inline struct __NODE_MEM_GUARD {
    atomic_uint32   __NODES;
    explicit __NODE_MEM_GUARD () : __NODES(0) {}
    ~__NODE_MEM_GUARD () {
        if (__NODES != 0)
            std::cerr<<"FIFO NODE MEMORY GUARD TRIGGER: The Iris Queue Node Memory Guard indicates that not all nodes within the queue have been destroyed. This indicates a MEMORY LEAK associated with the lockless queue structure. Place a breakpoint in the FIFO_Queue::QueueNode::DECREMENT and FIFO_Queue::__INTERNAL__Node destructor to track descrepencies between reference counts and destructor calls.";
    }
} NODE_MEMORY_GUARD;
#endif

constexpr uint16_t NODE_SIZE = 1<<11; //2048
constexpr uint16_t NODE_MASK = NODE_SIZE - 1;

template <class T>
class Queue;
template <class T>
class NodePtr;
template <class T>
class Iterator;
template <class T>
class __INTERNAL__Node;

template <class T>
using AtomicPtr = std::atomic<__INTERNAL__Node<T>*>;
template <class T>
using QueuePtr  = std::shared_ptr<Queue<T>>;

enum __EntryFlag {
    ENTRY_FREE,
    ENTRY_WRITING,
    ENTRY_PENDING,
    ENTRY_READING,
    ENTRY_COMPLETE
};

using EntryFlag = std::atomic<__EntryFlag>;
template <class T>
struct Entry {
    T                           handle;
    EntryFlag                   flag;
    explicit Entry              () :
    flag                        (ENTRY_FREE) {}
    static_assert(std::is_copy_constructible_v<T>, "Entries within Iris lockless queues must be copiable");
};

/// NodePtr is a
template <class T>
class NodePtr {
    friend class Queue<T>;
    friend class __INTERNAL__Node<T>;
protected:
    AtomicPtr<T>                _ptr;
public:
    // Default constructor, create NEW node
    NodePtr                     (AtomicPtr<T>& head_node);
    NodePtr                     (const NodePtr&);
    NodePtr                     (__INTERNAL__Node<T>*);
   ~NodePtr                     ();
    NodePtr& operator =         (const NodePtr&);
    bool compare_exchange       (const NodePtr& expected,
                                 const NodePtr& desired);
    
    __INTERNAL__Node<T>*
    operator *                  () const {
        return _ptr.load();
    }
    __INTERNAL__Node<T>*
    operator ->                 () const {
        return _ptr.load();
    }
    bool
    operator ==                 (const NodePtr& o) const {
        return _ptr == o._ptr;
    }
    bool
    operator ==                 (const __INTERNAL__Node<T>* ptr) const {
        return _ptr == ptr;
    }
    operator bool               () const {
        return _ptr.load() != nullptr;
    }
private:
    void set_reference          (__INTERNAL__Node<T>* __ptr);
};

template <class T>
class __INTERNAL__Node {
    friend class Queue<T>;
    friend class NodePtr<T>;
    friend class Iterator<T>;
    AtomicPtr<T>&               _head;
    Entry<T>                    _e[NODE_SIZE];
    atomic_uint16               _front;
    atomic_sint32               _use;
protected:
    NodePtr<T>                  _next;
    explicit __INTERNAL__Node   (AtomicPtr<T>& head) :
    _head                       (head),
    _front                      (0),
    _use                        (1),
    _next                       (NULL)
    {
    #if IRIS_DEBUG
        ++NODE_MEMORY_GUARD.__NODES;
    #endif
    }
    int16_t INCREMENT () {
        int use = _use.load (std::memory_order_acquire);
        while (use > 0)
            if (_use.compare_exchange_weak
                (use, use + 1,
                 std::memory_order_release,
                 std::memory_order_relaxed))
                return use;
        return 0;
    }
    int16_t DECREMENT () {
        return _use.fetch_sub(1)-1;
    }
public:
    __INTERNAL__Node            (const __INTERNAL__Node&) = delete;
    __INTERNAL__Node& operator =(const __INTERNAL__Node&) = delete;
   ~__INTERNAL__Node            ()
     {
       // If this is the head node, exchange the head node status \
       // with the next node in the chain.
       __INTERNAL__Node* NODE_PTR = this;
       _head.compare_exchange_strong(NODE_PTR, _next._ptr);
       
     #if IRIS_DEBUG
       for (uint16_t index = 0; index < NODE_SIZE; index++) {
           switch (_e[index].flag) {
               case ENTRY_FREE:
               case ENTRY_COMPLETE:
                   break;
               case ENTRY_WRITING:
               case ENTRY_PENDING:
               case ENTRY_READING:
                   std:: cerr << "Attempting to destory queue node "
                   << "with oustanding queue entry ["
                   << index
                   << "]; Place breakpoint in "
                   << __FILE__ << " at " << __LINE__
                   << " to debug. \n";
           }
       }
        --NODE_MEMORY_GUARD.__NODES;
     #endif
     }
    Entry<T>* get_front         ()
    {
        int16_t i = _front.fetch_add(1);
        return i < NODE_SIZE ? &_e[i] : nullptr;
    }
    Entry<T>* get_at            (uint16_t i)
    {
        return i < NODE_SIZE ? &_e[i] : nullptr;
    }
private:
    /// Extend chain is a THREAD SAFE call that will extend the chain by as many threads that call it.
    /// Guaranteed to create a new node when called and insert it in a thread-safe stack manner
    /// to the next chain.
    void extend_chain           ()
    {
        // END_TEST_NODE ensures NODE_TO_ADD_TO_CHAIN is only added to
        // the end of the growing chain. Each time the exchange fails
        // next is progressed down the growing chain.
        // NOTE: NODE_TO_ADD_TO_CHAIN reference count will be 1!
        // We are doing manual addition
        __INTERNAL__Node* END_TEST_NODE = nullptr;
        __INTERNAL__Node* NODE_TO_ADD_TO_CHAIN = new __INTERNAL__Node(_head);
        AtomicPtr<T>* next = &_next._ptr;
        while (!next->compare_exchange_weak(END_TEST_NODE,
                                            NODE_TO_ADD_TO_CHAIN,
                                            std::memory_order_release,
                                            std::memory_order_relaxed)) {
            // There is another link in the chain,
            // this link was returned in END_TEST_NODE.
            // Progress next with the addr of the next ptr in the chain
            // and reset the END_TEST_NODE to look for nullptr (ie end)
            next = &END_TEST_NODE->_next._ptr;
            END_TEST_NODE = nullptr;
        }
    }
};
// Default constructor, create NEW node
template <class T>
inline NodePtr<T>::NodePtr      (AtomicPtr<T>& head) :
_ptr                            (new __INTERNAL__Node(head))
{
    
}

// Copy constructor
template <class T>
inline NodePtr<T>::NodePtr      (const NodePtr<T>& o) :
_ptr                            (nullptr)
{
    set_reference(o._ptr);
}

// Construct from raw node ptr
template <class T>
inline NodePtr<T>::NodePtr      (__INTERNAL__Node<T>* __ptr) :
_ptr                            (nullptr)
{
    set_reference(__ptr);
}

// Copy assignment operator.
template <class T>
inline NodePtr<T>& NodePtr<T>::operator = (const NodePtr & o)
{
    // Perform an exchange storing the prior node
    // if there is a new node. Else set this to null.
    __INTERNAL__Node<T>* prior = nullptr;
    if (o && o._ptr.load()->INCREMENT() > 0)
         prior = _ptr.exchange(o._ptr);
    else prior = _ptr.exchange(nullptr);
    
    // Post exchange, decrement the prior
    // if var prior is the last reference
    // delete it.
    if (prior && prior->DECREMENT() < 1)
        delete prior;
    
    return *this;
}
// Destructor -- remove node if this is the last ptr
template <class T>
inline NodePtr<T>::~NodePtr             ()
{
    // If there is a nullptr for the reference
    if (_ptr.load() == nullptr) return;
    
    // If this is the last node access, delete it
    // Because this is a destructor we don't have to
    // worry about a copy exchange incrementing it.
    if (_ptr.load()->DECREMENT() < 1)
        delete _ptr.exchange(nullptr);
}
template <class T>
inline bool NodePtr<T>::compare_exchange (const NodePtr &expected,
                                          const NodePtr &desired)
{
    // Safety copy will ensure the reference count
    // will at least have 1, thus it will not require deletion
    // Until safety_copy exits scope (at the end of this call)
    NodePtr safty_copy  = expected;
    auto    EXPECTED    = safty_copy._ptr.load(std::memory_order_acquire);
    auto    DESIRED     = desired._ptr.load(std::memory_order_acquire);
    if (_ptr.compare_exchange_strong(EXPECTED, DESIRED,
                                     std::memory_order_release,
                                     std::memory_order_relaxed))
    {
        // Increment will increase the deired ptrs
        DESIRED->INCREMENT();
        // Decrement will NOT cause it to decrement to 0
        // because of the safety copy
        EXPECTED->DECREMENT();
        // This thread was successful in updating it, return true
        return true;
    }
    // This thread was unsuccessful; another thread made it here first.
    return false;
}
template <class T>
inline void NodePtr<T>::set_reference (__INTERNAL__Node<T>* __ptr) {
    // If this incremented
    if (__ptr && __ptr->INCREMENT() > 0)
         _ptr.store (__ptr);
    else _ptr.store (nullptr);
}

template <class T>
class Iterator {
    friend class Queue<T>;
    NodePtr<T>                  _node;
    uint16_t                    _index;
protected:
    explicit Iterator           (__INTERNAL__Node<T>* node_ptr) :
    _node                       (node_ptr),
    _index                      (0) {
        for (;;_index++) {
            // Grab the entry at the index
            Entry<T>* entry = _node->get_at(_index);
            
            // If there is no entry, move on to the next node.
            if (!entry) {
                // If we are at the end of the node and there is
                // no other node in the chain, assume it's the end.
                if (_node->_next == nullptr) return;
                
                // Grab the next node in the chain
                _node.compare_exchange(_node, _node->_next);
                _index  = 0;
                entry   = _node->get_at(_index);
            }
            
            switch (entry->flag) {
                // Most likely -- we are not at the end.
                // Either it is complete or we are at the reading end.
                // continue until we hit PENDING or
                case ENTRY_COMPLETE:
                case ENTRY_READING:
                    continue;
                    
                // PAST THE READING BLOCK TO THE PENDING BLOCK.
                // This is where we want iterators to begin.
                case ENTRY_PENDING:
                case ENTRY_FREE:
                case ENTRY_WRITING:
                    return;
            }
        }
    }
public:
    Iterator                    (const Iterator& o) :
    _node                       (o._node),
    _index                      (o._index){}
    bool pop                    (T& reference) {
        
        // This will always check the current node first
        // despite the fact it was likely previously emptied.
        for (;;_index++) {
            // Get the front entry (first)
            Entry<T>* entry = _node->get_at(_index);
            
            POP_ENTRY:
            if (entry) {
                __EntryFlag FLAG = ENTRY_PENDING;
                if (entry->flag.compare_exchange_strong(FLAG, ENTRY_READING)) {
                    reference       = entry->handle;
                    entry->handle   = T();
                    entry->flag.store(ENTRY_COMPLETE);
                    return true;
                    
                } switch (FLAG) {
                    // Most likely -- we are not at the end.
                    // Either it is complete or we are at the reading end.
                    // continue until we hit PENDING or
                    case ENTRY_COMPLETE:
                    case ENTRY_READING:
                        continue;
                        
                    // Spurrious failure; attempt it again.
                    // It's better to reattempt due to compare_exchange logic
                    case ENTRY_PENDING:
                        goto POP_ENTRY;
                      
                    // Writing Edge.
                    case ENTRY_FREE:
                    case ENTRY_WRITING:
                        return false;
                }
            }
            
            // If we are at the end of the node and there is
            // no other node in the chain, assume it's the end.
            if (_node->_next == nullptr) return false;
            
            // Grab the next node in the chain
            _node.compare_exchange(_node, _node->_next);
            _index  = 0;
            entry   = _node->get_at(_index);
            goto POP_ENTRY;
        }
    }
    bool at_end () {
        // Get the front entry (first)
        NodePtr<T> node = _node;
        Entry<T>* entry = _node->get_at(_index);
        
        if (!entry) {
            // If we are at the end of the node and there is
            // no other node in the chain, assume it's the end.
            if (node->_next == nullptr) return true;
            
            node    = node->_next;
            _index  = 0;
            entry   = node->get_at(_index);
        } switch (entry->flag.load(std::memory_order_acquire)) {
            case ENTRY_FREE:
                return true;
            case ENTRY_WRITING:
            case ENTRY_PENDING:
            case ENTRY_READING:
            case ENTRY_COMPLETE:
                return false;
        } return true;
    }
};

// FIRST IN FIRST OUT QUEUE:
// The queue has a STRONG reference to the tail node, which is at
// the growing end of the queue.
// There is a WEAK reference to the head at the back end of the queue.
// New "tail" nodes are added to the queue.
// New iterators are added and reference the lagging head as they work
// through the queue entries.
// Iterator -|                Queue -|
//           |                       |
//           v                       v
//          Head -> node -> node -> Tail -> next -> next -//
//            ^      ^
// Iterator 3-|      |
//       Iterator 2 -|
// iterator keeps the head alive. Queue maintains reference
// to Tail. Should Iterator 1 and 2 exit, the nodes will colapse
// until it hits Tail, which will stay alive due to Queue.
// Note: any 'next' nodes were created by threads needing additional
// nodes and are stored for use (see '__INTERNAL__Node::extend_chain()')
template <class T>
class Queue : public std::enable_shared_from_this<Queue<T>> {
    NodePtr<T>                  _tail;
    AtomicPtr<T>                _head;
public:
    // Generate the tail node as a new node, and reference
    // the tail node internal pointer as the head as well.
    explicit Queue              () :
    _tail                       (NodePtr(_head)),
    _head                       (_tail._ptr.load()) {}
    Queue                       (const Queue&) = delete;
    Queue& operator =           (const Queue&) = delete;
    void push                   (const T& reference)
    {
        auto tail = _tail;
        if (!tail) throw std::runtime_error("Failed to push entry. No valid tail\n");
        
        // Get the front entry (first) 
        Entry<T>* entry = tail->get_front();
        
    INSERT_ENTRY:
        if (entry) {
            switch (entry->flag.exchange(ENTRY_WRITING)) {
                // ANYTHING BESIDES A FREE SPACE IS A MISTAKE
                // Throw for development but attempt to recover in production
                case ENTRY_WRITING:
                case ENTRY_PENDING:
                case ENTRY_READING:
                case ENTRY_COMPLETE:
                    assert(false && "ERROR: Entry was not empty");
                    entry = tail->get_front();
                    goto INSERT_ENTRY;
                    
                // It should have been a free space
                case ENTRY_FREE:
                    entry->handle   = reference;
                    entry->flag     = ENTRY_PENDING;
                    return;
            }
        }
        
        while (entry == nullptr) {
            // Copy construct a shared_ptr to the next in the tail
            auto next = tail->_next;
            
            // If there is not another link in the chain,
            // Grow the chain. Each concurrent thread will add a link
            // any new unused links will be used in subsequent calls.
            // (chain extension will probably happen in bursts)
            if (next == nullptr) {
                tail->extend_chain();
                next = tail->_next;
                assert (next && "Chain extension failed.\n");
            }
            
            if (next) _tail.compare_exchange(tail, next);
            
            tail    = _tail;
            entry   = tail->get_front();
        }
        // A valid entry exits in var entry, go to insertion.
        goto INSERT_ENTRY;
    }
    Iterator<T> begin () const
    {
        return Iterator (_head.load(std::memory_order_acquire));
    }
};
} // END NAMESPACE FIFO

// MARK: - FILO QUEUE
namespace FILO2 {
using namespace Iris;
constexpr uint32_t QUEUE_SIZE = 0x80;//small for stress testing //1<<11; //2048
enum __EntryStatus {
    ENTRY_FREE,
    ENTRY_WRITING,
    ENTRY_PENDING,
    ENTRY_READING,
};
using EntryStatus = std::atomic<__EntryStatus>;
template <class T>
struct Entry {
    T               value;
    EntryStatus     status;
    explicit Entry  () :
    status(ENTRY_FREE)
    {
        
    }
    Entry           (const Entry&) = delete;
    Entry operator= (const Entry&) = delete;
    ~Entry          ()
    {
        assert(status == ENTRY_FREE);
    }
};
template <class T>
class Queue {
    static_assert(std::is_default_constructible<T>::value,  
                  "Entries into the queue must be default constructable");
    static_assert(std::is_copy_constructible<T>::value,     
                  "Entries into the queue must be copyable.");
    size_t              _size;
    Entry<T>*           _e;
    atomic_sint32       _i;
    SharedMutex         _resize;
    void RESIZE_INTERNAL(size_t new_size)
    {
        Iris::ExclusiveLock modify_lock (_resize);
        Entry<T>*   entries = static_cast<Entry<T>*>(std::realloc(_e, new_size * sizeof(Entry<T>)));
        #if IRIS_DEBUG
        assert(entries && "Failed to resize the queue block size");
        #else
        if (entries == nullptr) return;
        #endif
        new (entries+_size) Entry<T>[new_size-_size];
        _e      = entries;
        _size   = new_size;
        
    }
    void RAISE_INDEX_TO (int32_t new_index)
    {
        // Quick check
        assert (new_index > -1      && "Cannot assign an invalid index");
        assert (new_index < _size   && "Cannot assign an invalid index");
        
        // Attempt to raise the index to new_index unless it's less
        // than the current index.
        int32_t index = _i.load();
        while (new_index >= index)
            if (_i.compare_exchange_weak(index, new_index,
                                         std::memory_order_release,
                                         std::memory_order_relaxed))
                return;
    }
    bool FETCH_SUB      (int32_t& result)
    {
        result = _i.load();
        while (result > -1)
            if (_i.compare_exchange_weak(result, result-1,
                                         std::memory_order_release,
                                         std::memory_order_relaxed))
                return true;
        return false;
    }
public:
    explicit Queue      () :
    _size               (0),
    _e                  (nullptr),
    _i                  (-1)
    {
        RESIZE_INTERNAL(QUEUE_SIZE);
    }
   ~Queue               ()
    {
       Iris::ExclusiveLock destroy_lock (_resize);
       if (_e == NULL) return;
       
       for (uint32_t index = 0; index < _size; index++) {
           switch (_e[index].status) {
               case ENTRY_FREE:
                   break;
               case ENTRY_WRITING:
                   std:: cerr << "Attempting to destory queue node "
                   << "with oustanding queue WRITING entry ["
                   << index
                   << "]; Place breakpoint in "
                   << __FILE__ << " at " << __LINE__
                   << " to debug. \n";
                   break;
               case ENTRY_PENDING:
                   std:: cerr << "Attempting to destory queue node "
                   << "with oustanding queue PENDING entry ["
                   << index
                   << "]; Place breakpoint in "
                   << __FILE__ << " at " << __LINE__
                   << " to debug. \n";
                   break;
               case ENTRY_READING:
                   std:: cerr << "Attempting to destory queue node "
                   << "with oustanding queue READING entry ["
                   << index
                   << "]; Place breakpoint in "
                   << __FILE__ << " at " << __LINE__
                   << " to debug. \n";
                   break;
           }
       }
       
       // Free the queue memory
       std::free(_e);
       _e = nullptr;
    }
    Queue               (const Queue&) = delete;
    Queue& operator =   (const Queue&) = delete;
    void push           (const T& reference)
    {
        SharedLock      access_lock (_resize);
        int32_t         index   = _i.load();
        
        if (index < 0)  index   = 0;
        for (__EntryStatus STATUS = ENTRY_FREE;
             !_e[index].status.compare_exchange_strong
             (STATUS, ENTRY_WRITING,
              std::memory_order_release,
              std::memory_order_relaxed);
             STATUS = ENTRY_FREE)
        {
            if (index+1 >= _size) {
                access_lock.unlock();
                RESIZE_INTERNAL(_size * 2);
                access_lock.lock();
            }
            
            index += 1;
        }
        
        assert(_e[index].status == ENTRY_WRITING);
        _e[index].value     = reference;
        _e[index].status    = ENTRY_PENDING;
        RAISE_INDEX_TO (index);
    }
    bool pop            (T& reference)
    {
        SharedLock      access_lock (_resize);
        int32_t         index   = _i.load();
        
        for (__EntryStatus STATUS = ENTRY_PENDING;
             FETCH_SUB (index);
             STATUS = ENTRY_PENDING) {
            if (_e[index].status.compare_exchange_strong
                (STATUS, ENTRY_READING,
                 std::memory_order_release,
                 std::memory_order_relaxed))
            {
                reference           = _e[index].value;
                _e[index].value     = T();
                _e[index].status    = ENTRY_FREE;
                return true;
            }
        }
        return false;
    }
    bool at_end         () const
    {
        return _i.load(std::memory_order_acquire) == -1;
    }
    
};
} // END NAMESPACE FILE2
} // END NAMESPACE IRIS
#endif /* IrisQueueV2_h */
