import re


class Term:

    _var_pattern = re.compile('^<(\\d+)>$')

    @staticmethod
    def extract_var(name):
        m = Term._var_pattern.match(name)
        if m:
            return int(m.group(1))
        else:
            return None

    @staticmethod
    def name_var(v_id=1):
        return '<%d>' % v_id

    def __init__(self, label, children):
        self.label = label
        self.children = children

    def get_var_id(self):
        return Term.extract_var(str(self.label))

    def get_vars(self):
        if self.get_var_id() is not None:
            return [self]
        else:
            var_list = []
            for child in self.children:
                var_list.extend(child.get_vars())
            return var_list

    def is_const(self):
        return (not self.children) and (self.get_var_id() is None)

    @staticmethod
    def make_var(v_id=1):
        return Term(label=Term.name_var(v_id), children=[])

    def pretty_format(self, var_transform, op_transform, const_transform):
        if self.children:
            return op_transform(str(self.label),
                                *[child.pretty_format(var_transform, op_transform, const_transform)
                                  for child in self.children]
                                )
        else:
            var_id = self.get_var_id()
            if var_id is not None:
                return var_transform(self.label, var_id)
            else:
                return const_transform(self.label)

    def remap(self, labels_encoder):
        var_id = self.get_var_id()
        if var_id is not None:
            return self
        else:
            return Term(labels_encoder.encode(self.label), [child.remap(labels_encoder) for child in self.children])
    
    @staticmethod
    def var_transform(_, var_id):
        return 'x_%d' % var_id

    @staticmethod
    def op_transform(label, *children_str):
        return '%s(%s)' % (label, ', '.join(children_str))

    @staticmethod
    def const_transform(label):
        return str(label)

    def __str__(self):
        return self.pretty_format(
            Term.var_transform,
            Term.op_transform,
            Term.const_transform,
        )

    def __repr__(self):
        return str(self)
