import pyta
from pyta.automata.edit_transducer import edit_transducer
from pyta.automata import inter, inter_ac, scores_intersection, invhom, DynamicEncoder
from pyta.automata.semirings import inside_weight, MaxPlusSemiring, as_max_plus_element
from pyta.algebra.LeftConcAlgebra import decompose
import numpy as np
import numpy.random as rd
import string
from timeit import default_timer as timer
import matplotlib.pyplot as plt
from sklearn.preprocessing import PolynomialFeatures
from sklearn.pipeline import make_pipeline
from sklearn.linear_model import LinearRegression

def step(s1, s2):
    voc = set(s1).union(set(s2))
    t_sig = DynamicEncoder()
    (t, h1, h2, scores) = edit_transducer(voc, t_sig, transpositions=True, substitutions=True)
    d_sig = DynamicEncoder()
    d1 = decompose(s1, labels_encoder=d_sig)
    d2 = decompose(s2, labels_encoder=d_sig)
    inv1 = invhom(h1, d1, t_sig, d_sig)
    inv2 = invhom(h2, d2, t_sig, d_sig)
    start_t = timer()
    partial_chart = inter(inv1, inv2, t_sig, t.n_symb)
    chart = inter_ac(t, partial_chart, t_sig, t.n_symb)
    end_t = timer()
    #weights = scores_intersection(chart, scores_right=scores)
    #inside_map = {}
    #inside_weight(chart, weights, MaxPlusSemiring, as_max_plus_element, chart.final, inside_map)
    #print("d is {:d}".format(-inside_map[chart.final][0].get_value()))
    return end_t - start_t


def gen_seq(size):
    return ''.join(rd.choice(list(string.ascii_lowercase), size))


def gen_pair(size):
    return gen_seq(size), gen_seq(size)


def gen_data(n, min_size, max_size):
    points = []
    for s in range(min_size, max_size):
        points.append([gen_pair(s) for _ in range(n)])
    return points


M = 20
m = 0
n = 20
data = gen_data(n, m, M)
label = "C"
pyta.MODE = pyta.C
for k in range(2):
    Y = []
    for i in range(m, M):
        T = 0.
        for j in range(n):
            print("{:d}-{:d}/{:s} and {:s}".format(i, j, *data[i - m][j]))
            T += step(*data[i - m][j])
        Y.append(T/(M - m))
    plt.plot(range(m, M), Y, label=label)
    pyta.MODE = pyta.Python
    label = "Python"

polyreg = make_pipeline(PolynomialFeatures(2), LinearRegression())
X = np.array(range(m, M)).reshape(-1, 1)
print(X)
polyreg.fit(X, Y)
plt.plot(range(m, M), [polyreg.predict(np.array([x]).reshape(-1, 1)) for x in range(m, M)], label="deg 2 reg")
plt.ylabel("time (s)")
plt.xlabel("seq len")
plt.legend()
plt.show()


