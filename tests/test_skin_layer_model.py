# SPDX-License-Identifier: MIT
import importlib.util,math,unittest
from pathlib import Path
spec=importlib.util.spec_from_file_location('skin_layer_model',Path(__file__).parents[1]/'scripts/skin_layer_model.py')
model=importlib.util.module_from_spec(spec);spec.loader.exec_module(model)


class SkinLayerTests(unittest.TestCase):
    def setUp(self):
        self.bones={'eye.L':(3,-12,160),'eye.R':(-3,-12,160),
            'oris06':(0,-15,155),'oris05':(0,-15,154.5),
            'oris03.L':(1.4,-15,154.7),'oris03.R':(-1.4,-15,154.7)}
        self.marks=model.landmarks(self.bones)
    def test_channels_are_local_and_anatomically_separate(self):
        self.assertGreater(model.zones(self.marks['mouth'],self.marks)[0],.99)
        self.assertEqual(model.zones((0,-15,155),self.marks)[2],0)
        for point in [(0,0,155),(0,-10,100),(0,-10,140),(0,-10,173),(20,-14,160)]:
            self.assertEqual(model.zones(point,self.marks),(0.,0.,0.,0.))
    def test_transform_equivariance_of_local_binding(self):
        # If both anatomical anchors and skin move, the same UV point keeps its
        # weights. Holding world-space masks fixed is the rejecting control.
        p=(3.6,-12.8,156.9); old=model.zones(p,self.marks)
        delta=(.6,.2,2.)
        bones={k:tuple(a+b for a,b in zip(v,delta)) for k,v in self.bones.items()}
        new=model.zones(tuple(a+b for a,b in zip(p,delta)),model.landmarks(bones))
        for a,b in zip(old,new):self.assertAlmostEqual(a,b,places=12)
        self.assertGreater(abs(old[1]-model.zones(p,model.landmarks(bones))[1]),.1)
    def test_invalid_scale_and_nonfinite_are_rejected(self):
        with self.assertRaises(ValueError):model.bump((math.nan,0,0),(0,0,0),(1,1,1))
        with self.assertRaises(ValueError):model.bump((0,0,0),(0,0,0),(0,1,1))
        self.bones['eye.L']=(300,-12,160)
        with self.assertRaises(ValueError):model.landmarks(self.bones)

if __name__=='__main__':unittest.main()
