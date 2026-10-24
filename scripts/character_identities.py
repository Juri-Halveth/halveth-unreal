# SPDX-License-Identifier: MIT
"""Authored adult character definitions, not a biological or age simulation.

CC0 hm08 target data is loaded separately from a pinned source commit.  The
names of those source archetypes describe asset files, not real identities.
Only the adult endpoints (25 and 90) participate in the age blend.
"""
SOURCE_COMMIT = 'a8bc2d54ff0ac92e78ff71431b1023eda42bf482'
SOURCE_ROOT = ('https://raw.githubusercontent.com/makehumancommunity/makehuman/'
               + SOURCE_COMMIT + '/makehuman/data/targets/')
MORPHS = ('Blink', 'Talk', 'Breath', 'Smile',
          'IdentityJaw', 'IdentityNose', 'IdentityCheek', 'IdentityBrow')


def definition(identity, name, sex, age, height, source_mix, tint, proportions,
               face, brow, iris):
    return dict(identity=identity, name=name, sourceSex=sex, adultAgeYears=age,
                authoredHeightCm=height, sourceArchetypeMix=source_mix,
                skinSource='skin_' + sex, skinTintLinear=tint,
                proportions=proportions, faceTargets=face,
                browColorLinear=brow, irisColorLinear=iris)


# Body changes are regional and move anatomical helper vertices as well as
# visible skin, so the skeleton is rebuilt from each resulting anatomy.
IDENTITIES = (
    definition(1, 'Scarlet', 'female', 29, 171,
        {'caucasian': .75, 'asian': .15, 'african': .10}, [1., .93, .88],
        dict(shoulders=.94, hips=1.06, waist=.94, depth=.98, legs=.025, arms=.99),
        {'head/head-oval.target': .28, 'chin/chin-width-decr.target': .18,
         'nose/nose-point-up.target': .09, 'mouth/mouth-scale-horiz-incr.target': .10,
         'cheek/l-cheek-bones-incr.target': .20, 'cheek/r-cheek-bones-incr.target': .20},
        [.045, .018, .012], [.12, .21, .13]),
    definition(2, 'Lucinet', 'male', 36, 184,
        {'caucasian': .60, 'asian': .10, 'african': .30}, [.83, .70, .59],
        dict(shoulders=1.08, hips=.96, waist=.99, depth=1.06, legs=.005, arms=1.035),
        {'head/head-rectangular.target': .32, 'chin/chin-width-incr.target': .38,
         'chin/chin-prominent-incr.target': .16, 'nose/nose-curve-convex.target': .18,
         'mouth/mouth-upperlip-volume-incr.target': .12},
        [.025, .018, .012], [.11, .15, .19]),
    definition(3, 'Rachel', 'female', 34, 165,
        {'caucasian': .55, 'asian': .40, 'african': .05}, [.96, .86, .79],
        dict(shoulders=.92, hips=1.02, waist=.98, depth=1.01, legs=-.012, arms=.97),
        {'head/head-round.target': .22, 'chin/chin-width-incr.target': .10,
         'nose/nose-point-up.target': .13, 'mouth/mouth-scale-horiz-incr.target': .16,
         'cheek/l-cheek-volume-decr.target': .08, 'cheek/r-cheek-volume-decr.target': .08},
        [.075, .038, .019], [.20, .12, .065]),
    definition(4, 'Tala', 'female', 27, 178,
        {'caucasian': .10, 'asian': .15, 'african': .75}, [.44, .29, .19],
        dict(shoulders=1.02, hips=1.09, waist=.92, depth=1.02, legs=.045, arms=1.04),
        {'head/head-diamond.target': .28, 'nose/nose-width2-incr.target': .30,
         'mouth/mouth-upperlip-volume-incr.target': .30,
         'mouth/mouth-lowerlip-volume-incr.target': .28,
         'cheek/l-cheek-bones-incr.target': .22, 'cheek/r-cheek-bones-incr.target': .22},
        [.013, .010, .008], [.12, .065, .029]),
    definition(5, 'Orin', 'male', 54, 176,
        {'caucasian': .55, 'asian': .25, 'african': .20}, [.74, .56, .41],
        dict(shoulders=1.12, hips=1.04, waist=1.12, depth=1.14, legs=-.035, arms=.99),
        {'head/head-square.target': .38, 'chin/chin-width-incr.target': .42,
         'nose/nose-hump-incr.target': .25, 'nose/nose-scale-depth-incr.target': .20,
         'mouth/mouth-scale-horiz-incr.target': .12},
        [.095, .078, .062], [.16, .11, .07]),
    definition(6, 'Ivo', 'male', 43, 188,
        {'caucasian': .85, 'asian': .10, 'african': .05}, [.98, .91, .86],
        dict(shoulders=.98, hips=.94, waist=.90, depth=.94, legs=.035, arms=1.05),
        {'head/head-oval.target': .25, 'chin/chin-prominent-incr.target': .28,
         'nose/nose-curve-convex.target': .24,
         'cheek/l-cheek-volume-decr.target': .27, 'cheek/r-cheek-volume-decr.target': .27},
        [.09, .066, .042], [.15, .22, .25]),
    definition(7, 'Mara', 'female', 62, 160,
        {'caucasian': .25, 'asian': .25, 'african': .50}, [.60, .44, .31],
        dict(shoulders=1.01, hips=1.13, waist=1.13, depth=1.10, legs=-.025, arms=.975),
        {'head/head-round.target': .28, 'head/head-fat-incr.target': .15,
         'chin/chin-width-incr.target': .22, 'nose/nose-scale-depth-incr.target': .14,
         'mouth/mouth-scale-horiz-decr.target': .12},
        [.15, .13, .11], [.12, .08, .04]),
    definition(8, 'Neri', 'male', 25, 173,
        {'caucasian': .15, 'asian': .70, 'african': .15}, [.72, .61, .51],
        dict(shoulders=.96, hips=.99, waist=.93, depth=.96, legs=.01, arms=.99),
        {'head/head-triangular.target': .28, 'chin/chin-width-decr.target': .20,
         'nose/nose-curve-concave.target': .15,
         'mouth/mouth-upperlip-volume-incr.target': .12,
         'cheek/l-cheek-bones-incr.target': .28, 'cheek/r-cheek-bones-incr.target': .28},
        [.018, .014, .011], [.09, .06, .035]),
)


def macro_targets(character):
    """Convex blend of full adult archetypes; no partial sex-only face patch."""
    age = character['adultAgeYears']
    if not 25 <= age <= 90:
        raise ValueError('This generator accepts adult ages 25..90 only')
    mix = character['sourceArchetypeMix']
    if abs(sum(mix.values()) - 1.) > 1e-9 or any(v < 0 for v in mix.values()):
        raise ValueError('Source archetype mixture must be a convex combination')
    old = (age - 25.) / 65.
    result = {}
    for archetype, fraction in mix.items():
        for age_name, age_weight in (('young', 1.-old), ('old', old)):
            weight = fraction * age_weight
            if weight:
                path = f'macrodetails/{archetype}-{character["sourceSex"]}-{age_name}.target'
                result[path] = weight
    return result


def required_targets():
    return tuple(sorted({path for c in IDENTITIES
                         for path in (*macro_targets(c), *c['faceTargets'])}))
