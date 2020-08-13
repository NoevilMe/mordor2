#ifndef __MORDOR_NONCOPYABLE_H_
#define __MORDOR_NONCOPYABLE_H_

namespace Mordor2 {

class Noncopyable {
protected:
    Noncopyable(const Noncopyable &) = delete;
    void operator=(const Noncopyable &) = delete;

protected:
    Noncopyable() = default;
    ~Noncopyable() = default;
};

} // namespace Mordor2

#endif /* __NONCOPYABLE_H_ */
