from pyta.edit_distance.edit_transducer import INSERTION_PFX, DELETION_PFX, INIT_TR_PFX, CLOSE_TR_PFX, SUB_PFX
import re

extract_letters = re.compile('^\\w*\\((.)(?:\\|(.))?\\)$')


def decode(a, back_pointers, start, seq):
    value = back_pointers[start]
    rule_name = value[1]
    score = value[0]
    rule = a.get_rule(rule_name)
    seq.append((a.labels_decoder.decode(rule[1]), score))
    if rule[2]:
        next_state = rule[2][0]
        decode(a, back_pointers, next_state, seq)


# Fixme: this is pretty ugly. Must be a better way, or better organization to the least
def annotate_time_steps(decoded_seq):
    # precompute number of deletion between each pair of init_tr and close_tr rule.
    del_counts = []
    current_count = 0
    counting = False
    for (rule_label, _) in decoded_seq:
        if rule_label.startswith(INIT_TR_PFX):
            counting = True

        elif rule_label.startswith(DELETION_PFX) and counting:
            current_count += 1

        elif rule_label.startswith(CLOSE_TR_PFX):
            del_counts.append(current_count)
            current_count = 0
            counting = False

    # because we're gonna use pop()
    del_counts.reverse()

    # compute the time step at which each rewrite operation happens
    n = len(decoded_seq)
    time_steps = [n] * n
    waiting_tr = False
    current_step = 1
    offset = 0
    del_seen = 0
    ins_seen = 0

    for i in range(n):
        rule_label = decoded_seq[i][0]
        if rule_label.startswith(INIT_TR_PFX):
            waiting_tr = True
            offset = del_counts.pop()
            time_steps[i] = current_step + offset
            del_seen = 0
            ins_seen = 0

        elif rule_label.startswith(INSERTION_PFX) and waiting_tr:
            time_steps[i] = current_step + offset + ins_seen + 1
            ins_seen += 1

        elif rule_label.startswith(DELETION_PFX) and waiting_tr:
            time_steps[i] = current_step + del_seen
            del_seen += 1

        elif rule_label.startswith(CLOSE_TR_PFX):
            waiting_tr = False
            time_steps[i] = current_step + offset
            current_step = current_step + offset + ins_seen + 1

        else:
            time_steps[i] = current_step
            current_step += 1

    return time_steps


# TODO: reading the homomorphic images would be more elegant and equivalent...
def extract_alignment(decoded_seq):
    n = len(decoded_seq)

    original_buffer = [''] * n
    target_buffer = [''] * n

    for seq_index in range(n):
        next_rule_label = decoded_seq[seq_index][0]

        if next_rule_label.startswith(INSERTION_PFX):
            l = extract_letters.match(next_rule_label).group(1)
            target_buffer[seq_index] = l

        elif next_rule_label.startswith(DELETION_PFX):
            l = extract_letters.match(next_rule_label).group(1)
            original_buffer[seq_index] = l

        elif next_rule_label.startswith(INIT_TR_PFX) or next_rule_label.startswith(SUB_PFX):
            m = extract_letters.match(next_rule_label)
            l, r = m.group(1), m.group(2)
            original_buffer[seq_index] = l
            target_buffer[seq_index] = r

        elif next_rule_label.startswith(CLOSE_TR_PFX):
            m = extract_letters.match(next_rule_label)
            l, r = m.group(1), m.group(2)
            original_buffer[seq_index] = r
            target_buffer[seq_index] = l

    return original_buffer, target_buffer


def rewrite_at(t, original, target, time_steps):
    rewrite = []
    for i in range(len(original)):
        if t >= time_steps[i]:
            rewrite.append(target[i])
        else:
            rewrite.append(original[i])

    return ''.join(rewrite)


def read_rewrite_seq(decoded_seq):
    # reading the rewritten string at each timestep
    rs = []
    time_steps = annotate_time_steps(decoded_seq)
    original, target = extract_alignment(decoded_seq)

    n_steps = max(time_steps)
    # note: the last time step is always the final rule who does not bring any change (and has cost 0)
    for i in range(n_steps):
        if i == 0 or original[i-1] != target[i-1]:
            rs.append(rewrite_at(i, original, target, time_steps))

    # reading the rewrite ops
    ops = []
    n = len(decoded_seq)
    ordered_indices = list(range(n))
    ordered_indices.sort(key=lambda k: (time_steps[k], k))

    step = 1
    j = 0
    while step < n_steps:
        incr = 1
        if j < n - 1 and time_steps[ordered_indices[j]] == time_steps[ordered_indices[j+1]]:
            ops.append('transpose %d <-> %d' % (ordered_indices[j], ordered_indices[j]+1))
            incr = 2
        elif original[ordered_indices[j]] and target[ordered_indices[j]]:
            if original[ordered_indices[j]] != target[ordered_indices[j]]:
                ops.append('substitute %s @ %d' % (target[ordered_indices[j]], ordered_indices[j]))
        elif not original[ordered_indices[j]]:
            ops.append('insert %s @ %d' % (target[ordered_indices[j]], ordered_indices[j]))
        else:
            ops.append('delete @ %d' % ordered_indices[j])
        step += 1
        j += incr

    return rs, ops
