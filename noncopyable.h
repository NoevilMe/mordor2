#ifndef __NONCOPYABLE_H_
#define __NONCOPYABLE_H_

namespace Mordor {

class Noncopyable {
protected:
    Noncopyable(const Noncopyable &) = delete;
    void operator=(const Noncopyable &) = delete;

protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};

} // namespace Mordor

#endif /* __NONCOPYABLE_H_ */
