#!/usr/bin/python
# -*- coding: utf-8 -*-

"""
This module handles the clustering, from a general matter.
We have a list of elements and a structure containing their distance,
like dists[p][q] = dists[q][p] = || q - p ||::

    >>> points = [1, 2, 3, 4, 5, 6, 7]
    >>> dists = build_dists(points, custom_dist)
    >>> dists
    {1: {1: 0, 2: 1, 3: 2, 4: 3, 5: 4, 6: 5, 7: 6}, 2: {1: 1,...


Then we have an implementation of the k-medoids algorithm::

    >>> medoids, diam = k_medoids_iterspawn(points, k=2, dists=dists, spawn=2) #doctest: +SKIP
    -- Spawning
    * New chosen kernels: [7, 5]
    * Iteration over after 4 steps
    * Diameter max 3.000 (1, 4) and medoids: {'m1': [2, [1, 2, 3, 4]], 'm0': [6, [5, 6, 7]]}
    -- Spawning
    * New chosen kernels: [1, 6]
    * Iteration over after 2 steps
    * Diameter max 3.000 (4, 7) and medoids: {'m1': [5, [4, 5, 6, 7]], 'm0': [2, [1, 2, 3]]}
    -- Spawn end: best diameter 3.000, best medoids: {'m1': [2, [1, 2, 3, 4]], 'm0': [6, [5, 6, 7]]}
    >>>
    >>> for kernel, medoid in medoids.itervalues():
    ...     print "%s -> %s" % (kernel, medoid) #doctest: +SKIP
    5 -> [4, 5, 6, 7]
    1 -> [1, 2, 3]

And a version which increases automatically the number of clusters till
we have homogeneous clusters::

    >>> medoids, diam = k_medoids_iterall(points, diam_max=3, dists=dists, spawn=2) #doctest: +SKIP
    -- Spawning
    * New chosen kernels: [1]
    * Iteration over after 2 steps
    * Diameter max 6.000 (1, 7) and medoids: {'m0': [4, [1, 2, 3, 4, 5, 6, 7]]}
    -- Spawning
    * New chosen kernels: [4]
    * Iteration over after 1 steps
    * Diameter max 6.000 (1, 7) and medoids: {'m0': [4, [1, 2, 3, 4, 5, 6, 7]]}
    -- Spawn end: best diameter 6.000, best medoids: {'m0': [4, [1, 2, 3, 4, 5, 6, 7]]}
    +++ Diameter too bad 6.000 > 3.000
    +++ Going to 2 clusters
    <BLANKLINE>
    -- Spawning
    * New chosen kernels: [7, 5]
    * Iteration over after 4 steps
    * Diameter max 3.000 (1, 4) and medoids: {'m1': [2, [1, 2, 3, 4]], 'm0': [6, [5, 6, 7]]}
    -- Spawning
    * New chosen kernels: [5, 1]
    * Iteration over after 2 steps
    * Diameter max 3.000 (4, 7) and medoids: {'m1': [2, [1, 2, 3]], 'm0': [5, [4, 5, 6, 7]]}
    -- Spawn end: best diameter 3.000, best medoids: {'m1': [2, [1, 2, 3, 4]], 'm0': [6, [5, 6, 7]]}
    +++ Diameter ok 3.000 ~ 3.000
    +++ Stopping, 2 clusters enough (7 points initially)

"""


import random


def draw_without_replacement(points, draw):
    """
    Draw different elements in points, randomly
    and without putting back the chosen ones.

    :param points:  the list of points
    :param draw:    an integer, the number of draw we want
    :returns:       the iterator of chosen points

    >>> L = ['a', 'b', 'c']
    >>> list(draw_without_replacement(L, 0))
    []
    >>> list(draw_without_replacement(L, 1))
    ['...']
    >>> list(draw_without_replacement(L, 2))
    ['...', '...']
    >>> L
    ['a', 'b', 'c']
    """

    # That is simple :)
    return random.sample(points, draw)



def custom_dist(a, b):
    """
    That is a good one, example purpose.

    :param a: number a
    :param b: number b
    :returns: absolute value of (b - a)

    >>> custom_dist(2, 3)
    1
    >>> custom_dist(-2.5, 0)
    2.5
    """

    return abs(b - a)


def build_dists(points, distance):
    """
    From a list of elements and a function to compute
    the distance between two of them, build the necessary structure
    which is required to perform k-medoids.
    This is like a cache of all distances between all points.

    :param points:   the list of elements
    :param distance: a function able to do distance(a, b) = || b - a ||
    :returns:        the structure containing all distances, it is \
        a dictionary of dictionary, that verifies: \
         res[a][b] = res[b][a] = distance(a, b)

    >>> points = [1, 2, 3]
    >>> from pprint import pprint
    >>> pprint(build_dists(points, custom_dist), width=60)
    {1: {1: 0, 2: 1, 3: 2},
     2: {1: 1, 2: 0, 3: 1},
     3: {1: 2, 2: 1, 3: 0}}
    """
    dists = {}

    for p in points:
        dists[p] = {}

    for p in points:
        for q in points:
            dists[p][q] = distance(p, q)
            dists[q][p] = dists[p][q]

    return dists


def _init_kernel_and_medoids(points, k, verbose=1):
    """Initialize kernels and medoids.

    :param points:  the list of points
    :param k:       the number of clusters
    :param verbose: verbosity
    :returns:       the medoids, structured as \
        a dictionary of 'medoid name' : medoids, \
        where a medoid is [kernel, [elements...]]

    >>> _init_kernel_and_medoids([1, 2, 3], 2) #doctest: +SKIP
    * New chosen kernels: [1, 3]
    {'m1': [3, []], 'm0': [1, []]}
    >>> _init_kernel_and_medoids([1, 2, 3], 4)
    * Too much clusters, fixing with k=3
    * New chosen kernels: ...
    {...}
    """

    if k > len(points):
        k = len(points)

        if verbose:
            print '* Too much clusters, fixing with k=%s' % k

    kernels = draw_without_replacement(points, k)

    if verbose:
        print '* New chosen kernels: %s' % kernels

    return dict([
        ("m%s" % i, [kernels[i], []])
        for i in xrange(len(kernels))
    ])


def _reset_medoids(medoids):
    """Reset medoids.

    :param medoids: the medoids, structured as \
        a dictionary of 'medoid name' : medoids, \
        where a medoid is [kernel, [elements...]]
    :returns:       None

    >>> medoids = {'m1': [3, [2, 3]], 'm0': [1, [1]]}
    >>> _reset_medoids(medoids)
    >>> medoids
    {'m1': [3, []], 'm0': [1, []]}
    """

    for m in medoids:
        medoids[m][1] = []


def _put_points_in_closest_medoid(points, dists, medoids, verbose=1):
    """Put points in closest medoids.

    :param points:  the list of points
    :param dists:   the cache of distances dists[p][q] = || q - p ||
    :param medoids: the medoids, structured as \
        a dictionary of 'medoid name' : medoids, \
        where a medoid is [kernel, [elements...]]
    :param verbose: verbosity
    :returns:       None

    >>> points = [1, 2, 3]
    >>> medoids = {'m1': [3, []], 'm0': [1, []]}
    >>> _put_points_in_closest_medoid(points, dists, medoids, verbose=2)
    Point 1 at dist. 0.00 of kernel 1: 1 goes in medoid [1, []]
    Point 2 at dist. 1.00 of kernel 1: 2 goes in medoid [1, [1]]
    Point 3 at dist. 0.00 of kernel 3: 3 goes in medoid [3, []]
    >>> medoids
    {'m1': [3, [3]], 'm0': [1, [1, 2]]}
    """

    for p in points:

        dist, closest_m = min(
             (dists[p][k], m)
             for m, (k, el) in medoids.iteritems()
        )

        if verbose >= 2:
            print 'Point %s at dist. %.2f of kernel %s: %s goes in medoid %s' % \
                (p, dist, medoids[closest_m][0], p, medoids[closest_m])

        medoids[closest_m][1].append(p)


def _remove_empty_medoids(medoids, verbose=1):
    """
    Remove empty medoids, happens only if there are
    points with null distances, normally duplicates.

    :param medoids: the medoids, structured as \
        a dictionary of 'medoid name' : medoids, \
        where a medoid is [kernel, [elements...]]
    :param verbose: verbosity
    :returns:       None

    >>> points = [1, 2, 3]
    >>> medoids = {'m1': [3, [1, 2, 3]], 'm0': [1, []]}
    >>> _remove_empty_medoids(medoids)
    * Removing medoid [1, []]
    >>> medoids
    {'m1': [3, [1, 2, 3]]}
    """

    empty_medoids = []

    for m, (kernel, elements) in medoids.iteritems():

        if not elements:

            if verbose:
                print '* Removing medoid %s' % ([kernel, elements])

            empty_medoids.append(m)

    # Dictionary, so this will
    # remove all empty medoids indeed
    for key in empty_medoids:
        medoids.pop(key)


def _elect_new_kernels(dists, medoids, verbose=1):
    """Electing new kernels for each medoid.

    :param dists:   the cache of distances dists[p][q] = || q - p ||
    :param medoids: the medoids, structured as \
        a dictionary of 'medoid name' : medoids, \
        where a medoid is [kernel, [elements...]]
    :param verbose: verbosity
    :returns:       boolean, True if there is a change \
        in the kernel of the medoids

    >>> points = [1, 2, 3]
    >>> medoids = {'m1': [3, [2, 3]], 'm0': [1, [1]]}
    >>> change = _elect_new_kernels(dists, medoids, verbose=2)
    Electing new kernel 2 for medoid [3, [2, 3]]
    Kernel 1 still best for medoid [1, [1]]
    >>> medoids
    {'m1': [2, [2, 3]], 'm0': [1, [1]]}
    """
    #
    # Electing new kernel for the medoid
    #
    change = False

    for m in medoids:
        elements = medoids[m][1]

        # pk = potential kernel
        new_kernel = min(
            (sum(dists[e][pk] for e in elements), pk)
            for pk in elements
        )[1]

        if medoids[m][0] != new_kernel:
            if verbose >= 2:
                print 'Electing new kernel %s for medoid %s' % (new_kernel, medoids[m])

            medoids[m][0] = new_kernel
            change = True

        else:
            if verbose >= 2:
                print 'Kernel %s still best for medoid %s' % (new_kernel, medoids[m])

    return change


def _compute_diameter_max(dists, medoids, verbose=1):
    """Compute diameter max

    :param dists:   the cache of distances dists[p][q] = || q - p ||
    :param medoids: the medoids, structured as \
        a dictionary of 'medoid name' : medoids, \
        where a medoid is [kernel, [elements...]]
    :param verbose: verbosity
    :returns:       the max diameter, a tuple of \
        (diam, couple) where couple has a distance of diameter

    >>> medoids = {'m1': [2, [2, 3]], 'm0': [1, [1]]}
    >>> diam = _compute_diameter_max(dists, medoids)
    * Diameter max 1.000 (3, 2) and medoids: {'m1': [2, [2, 3]], 'm0': [1, [1]]}
    >>> medoids
    {'m1': [2, [2, 3]], 'm0': [1, [1]]}
    """

    diam, couple = max(
        (dists[a][b], (a, b))
        for kernel, elements in medoids.itervalues()
        for a in elements
        for b in elements
    )

    if verbose:
        print '* Diameter max %.3f %s and medoids: %s' % (diam, couple, medoids)

    return diam


def k_medoids(points, k, dists, iteration=20, verbose=1):
    """Standard k-medoids algorithm.

    :param points:    the list of points
    :param k:         the number of clusters
    :param dists:     the cache of distances dists[p][q] = || q - p ||
    :param iteration: the maximum number of iterations
    :param verbose:   verbosity
    :returns:         the partition, structured as \
        a list of [kernel of the cluster, [elements in the cluster]]

    >>> diam, medoids = k_medoids(points, k=2, dists=dists, verbose=2) #doctest: +SKIP
    -- Spawning
    * New chosen kernels: [4, 6]
    Point 1 at dist. 3.00 of kernel 4: 1 goes in medoid [4, []]
    Point 2 at dist. 2.00 of kernel 4: 2 goes in medoid [4, [1]]
    Point 3 at dist. 1.00 of kernel 4: 3 goes in medoid [4, [1, 2]]
    Point 4 at dist. 0.00 of kernel 4: 4 goes in medoid [4, [1, 2, 3]]
    Point 5 at dist. 1.00 of kernel 4: 5 goes in medoid [4, [1, 2, 3, 4]]
    Point 6 at dist. 0.00 of kernel 6: 6 goes in medoid [6, []]
    Point 7 at dist. 1.00 of kernel 6: 7 goes in medoid [6, [6]]
    Electing new kernel 3 for medoid [4, [1, 2, 3, 4, 5]]
    Kernel 6 still best for medoid [6, [6, 7]]
    Point 1 at dist. 2.00 of kernel 3: 1 goes in medoid [3, []]
    Point 2 at dist. 1.00 of kernel 3: 2 goes in medoid [3, [1]]
    Point 3 at dist. 0.00 of kernel 3: 3 goes in medoid [3, [1, 2]]
    Point 4 at dist. 1.00 of kernel 3: 4 goes in medoid [3, [1, 2, 3]]
    Point 5 at dist. 1.00 of kernel 6: 5 goes in medoid [6, []]
    Point 6 at dist. 0.00 of kernel 6: 6 goes in medoid [6, [5]]
    Point 7 at dist. 1.00 of kernel 6: 7 goes in medoid [6, [5, 6]]
    Electing new kernel 2 for medoid [3, [1, 2, 3, 4]]
    Kernel 6 still best for medoid [6, [5, 6, 7]]
    Point 1 at dist. 1.00 of kernel 2: 1 goes in medoid [2, []]
    Point 2 at dist. 0.00 of kernel 2: 2 goes in medoid [2, [1]]
    Point 3 at dist. 1.00 of kernel 2: 3 goes in medoid [2, [1, 2]]
    Point 4 at dist. 2.00 of kernel 2: 4 goes in medoid [2, [1, 2, 3]]
    Point 5 at dist. 1.00 of kernel 6: 5 goes in medoid [6, []]
    Point 6 at dist. 0.00 of kernel 6: 6 goes in medoid [6, [5]]
    Point 7 at dist. 1.00 of kernel 6: 7 goes in medoid [6, [5, 6]]
    Kernel 2 still best for medoid [2, [1, 2, 3, 4]]
    Kernel 6 still best for medoid [6, [5, 6, 7]]
    * Iteration over after 3 steps
    * Diameter max 3.000 (1, 4) and medoids: {'m0': [2, [1, 2, 3, 4]], 'm1': [6, [5, 6, 7]]}
    >>>
    >>> for kernel, medoid in medoids.itervalues():
    ...     print "%s -> %s" % (kernel, medoid) #doctest: +SKIP
    2 -> [1, 2, 3, 4]
    6 -> [5, 6, 7]
    """

    if verbose:
        print '-- Spawning'

    medoids = _init_kernel_and_medoids(points, k, verbose=verbose)

    for n in xrange(iteration):

        # Attributing closest kernels
        _reset_medoids(medoids)
        _put_points_in_closest_medoid(points, dists, medoids, verbose=verbose)
        _remove_empty_medoids(medoids, verbose=verbose)

        # Electing new kernel for the kernel
        change = _elect_new_kernels(dists, medoids, verbose=verbose)

        if not change:
            if verbose:
                print '* Iteration over after %s steps' % (1+n)
            break

    return _compute_diameter_max(dists, medoids, verbose=verbose), medoids


def k_medoids_iterspawn(points, k, dists, spawn=1, iteration=20, verbose=1):
    """
    Same as k_medoids, but we iterate also
    the spawning process.
    We keep the minimum of the biggest diam as a
    reference for the best spawn.

    :param points:    the list of points
    :param k:         the number of clusters
    :param dists:     the cache of distances dists[p][q] = || q - p ||
    :param spawn:     the number of spawns
    :param iteration: the maximum number of iterations
    :param verbose:   boolean, verbosity status
    :returns:         the partition, structured as \
        a list of [kernel of the cluster, [elements in the cluster]]

    >>> diam, medoids = k_medoids_iterspawn(points, k=2, dists=dists, spawn=2) #doctest: +SKIP
    -- Spawning
    * New chosen kernels: [2, 3]
    * Iteration over after 3 steps
    * Diameter max 3.000 (4, 7) and medoids: {'m0': [2, [1, 2, 3]], 'm1': [5, [4, 5, 6, 7]]}
    -- Spawning
    * New chosen kernels: [4, 1]
    * Iteration over after 2 steps
    * Diameter max 4.000 (3, 7) and medoids: {'m0': [5, [3, 4, 5, 6, 7]], 'm1': [1, [1, 2]]}
    -- Spawn end: best diameter 3.000, best medoids: {'m0': [2, [1, 2, 3]], 'm1': [5, [4, 5, 6, 7]]}
    >>>
    >>> for kernel, medoid in medoids.itervalues():
    ...     print "%s -> %s" % (kernel, medoid) #doctest: +SKIP
    2 -> [1, 2, 3]
    5 -> [4, 5, 6, 7]
    """

    # Here the result of k_medoids function is a tuple
    # containing in the second element the diameter of the
    # biggest medoid, so the min function will return
    # the best medoids arrangement, in the sense that the
    # diameter max will ne minimum
    diam, medoids = min((k_medoids(points, k, dists, iteration, verbose) for _ in xrange(spawn)),
                        key=lambda km: km[0])

    if verbose:
        print '-- Spawn end: best diameter %.3f, best medoids: %s' % (diam, medoids)

    return diam, medoids


def k_medoids_iterall(points, diam_max, dists, spawn=1, iteration=20, verbose=1):
    """
    Same as k_medoids_iterspawn, but we increase
    the number of clusters till  we have a good enough
    similarity between paths.

    :param points:     the list of points
    :param diam_max: the maximum diameter allowed, otherwise \
        the algorithm will start over and increment the number of clusters
    :param dists:      the cache of distances dists[p][q] = || q - p ||
    :param spawn:      the number of spawns
    :param iteration:  the maximum number of iterations
    :param verbose:    verbosity
    :returns:          the partition, structured as \
        a list of [kernel of the cluster, [elements in the cluster]]

    >>> diam, medoids = k_medoids_iterall(points, diam_max=3, dists=dists, spawn=3)  #doctest: +SKIP
    -- Spawning
    * New chosen kernels: [3]
    * Iteration over after 2 steps
    * Diameter max 6.000 (1, 7) and medoids: {'m0': [4, [1, 2, 3, 4, 5, 6, 7]]}
    -- Spawning
    * New chosen kernels: [3]
    * Iteration over after 2 steps
    * Diameter max 6.000 (1, 7) and medoids: {'m0': [4, [1, 2, 3, 4, 5, 6, 7]]}
    -- Spawning
    * New chosen kernels: [7]
    * Iteration over after 2 steps
    * Diameter max 6.000 (1, 7) and medoids: {'m0': [4, [1, 2, 3, 4, 5, 6, 7]]}
    -- Spawn end: best diameter 6.000, best medoids: {'m0': [4, [1, 2, 3, 4, 5, 6, 7]]}
    +++ Diameter too bad 6.000 > 3.000
    +++ Going to 2 clusters
    <BLANKLINE>
    * New chosen kernels: [5, 4]
    * Iteration over after 3 steps
    * Diameter max 3.000 (4, 7) and medoids: {'m0': [5, [4, 5, 6, 7]], 'm1': [2, [1, 2, 3]]}
    -- Spawning
    * New chosen kernels: [6, 1]
    * Iteration over after 2 steps
    * Diameter max 3.000 (4, 7) and medoids: {'m0': [5, [4, 5, 6, 7]], 'm1': [2, [1, 2, 3]]}
    -- Spawning
    * New chosen kernels: [4, 1]
    * Iteration over after 2 steps
    * Diameter max 4.000 (3, 7) and medoids: {'m0': [5, [3, 4, 5, 6, 7]], 'm1': [1, [1, 2]]}
    -- Spawn end: best diameter 3.000, best medoids: {'m0': [5, [4, 5, 6, 7]], 'm1': [2, [1, 2, 3]]}
    +++ Diameter ok 3.000 ~ 3.000
    +++ Stopping, 2 clusters enough (7 points initially)
    >>>
    >>> for kernel, medoid in medoids.itervalues():
    ...     print "%s -> %s" % (kernel, medoid) #doctest: +SKIP
    2 -> [1, 2, 3]
    5 -> [4, 5, 6, 7]
    >>>
    >>> diam, medoids = k_medoids_iterall(points, diam_max=3, dists=dists, spawn=10, verbose=0)
    >>> diam <= 3
    True
    >>> len(medoids)
    2
    """

    # Some precautions, verbose must be an int
    verbose = (1 if verbose else 0)

    if not points:
        return -1, {}

    for k in xrange(len(points)):
        diam, medoids = k_medoids_iterspawn(points, 1+k, dists, spawn, iteration, verbose)

        if diam <= diam_max:
            break

        if verbose:
            print '+++ Diameter too bad %.3f > %.3f' % (diam, diam_max)
            print '+++ Going to %s clusters\n' % (2+k)

    if verbose:
        print '+++ Diameter ok %.3f ~ %.3f' % (diam, diam_max)
        print '+++ Stopping, %s clusters enough (%s points initially)' % (1+k, len(points))

    return diam, medoids


def _test():
    """
    When called directly, launching doctests.
    """
    import doctest

    opt =  (doctest.ELLIPSIS |
            doctest.NORMALIZE_WHITESPACE |
            doctest.REPORT_ONLY_FIRST_FAILURE )
            #doctest.IGNORE_EXCEPTION_DETAIL)

    points = [1, 2, 3, 4, 5, 6, 7]
    dists  = build_dists(points, custom_dist)

    globs = {
        'points': points,
        'dists' : dists
    }

    doctest.testmod(optionflags=opt,
                    extraglobs=globs,
                    verbose=False)



if __name__ == '__main__':
    _test()

