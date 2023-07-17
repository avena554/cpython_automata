def cartesian_product(*args):
    if args:
        iterators = [iter(c) for c in args]
        v_pos = len(args) - 1
        try:
            next_item = [next(i) for i in iterators[:-1]]
            next_item.append(None)

            while v_pos >= 0:
                try:
                    next_item[v_pos] = next(iterators[v_pos])
                    yield tuple(next_item)
                    if v_pos < len(args) - 1:
                        v_pos += 1
                except StopIteration:
                    iterators[v_pos] = iter(args[v_pos])
                    next_item[v_pos] = next(iterators[v_pos])
                    v_pos -= 1

        except StopIteration:
            pass
    else:
        yield ()
