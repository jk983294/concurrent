#include <algorithm>
#include <iostream>
#include <memory>
#include <mutex>
#include <vector>

class Observable;

class Observer : public std::enable_shared_from_this<Observer> {
public:
    virtual ~Observer() = default;
    virtual void update() = 0;

    void observe(Observable* s);

protected:
    Observable* subject_;
};

class Observable {
public:
    void register_(const std::weak_ptr<Observer>& x) { observers_.push_back(x); }

    void notifyObservers() {
        std::lock_guard<std::mutex> lock(mutex_);
        Iterator it = observers_.begin();
        while (it != observers_.end()) {
            std::shared_ptr<Observer> obj(it->lock());  // try to promote, it is thread safe
            if (obj) {
                obj->update();
                ++it;
            } else {  // observer object already gone, remove it
                printf("notifyObservers() erase\n");
                it = observers_.erase(it);
            }
        }
    }

private:
    mutable std::mutex mutex_;
    std::vector<std::weak_ptr<Observer> > observers_;
    typedef std::vector<std::weak_ptr<Observer> >::iterator Iterator;
};

void Observer::observe(Observable* s) {
    s->register_(shared_from_this());
    subject_ = s;
}

class Foo : public Observer {
    virtual void update() { std::cout << "Foo::update()\n"; }
};

int main() {
    Observable subject;
    {
        std::shared_ptr<Foo> p(new Foo);
        p->observe(&subject);
        subject.notifyObservers();
    }
    subject.notifyObservers();
    return 0;
}
