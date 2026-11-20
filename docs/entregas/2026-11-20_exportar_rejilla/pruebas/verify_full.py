import sys, numpy as np, cv2
grid_dir, bayer_dir, iso = sys.argv[1], sys.argv[2], sys.argv[3]
full = [(1486,560),(1403,3823),(5999,3993),(6113,667)]   # -x order handled by ChartProfile: TL, BL, BR, TR
src = np.array([(x/2.0, y/2.0) for x, y in full])
xs, ys = src[:,0], src[:,1]
dst = np.array([(xs.min(), ys.min()), (xs.min(), ys.max()), (xs.max(), ys.max()), (xs.max(), ys.min())])
A = np.zeros((8,8)); b = np.zeros(8)
for i in range(4):
    xu, yu = src[i]; xd, yd = dst[i]
    A[2*i]   = [xd, yd, 1, 0, 0, 0, -xd*xu, -yd*xu]; b[2*i] = xu
    A[2*i+1] = [0, 0, 0, xd, yd, 1, -xd*yu, -yd*yu]; b[2*i+1] = yu
k = np.linalg.lstsq(A, b, rcond=None)[0]
cr = lambda v: np.floor(v + 0.5)  # C++ round for positive values
x0, y0 = int(cr(dst[0,0])), int(cr(dst[0,1])); w = int(cr(dst[2,0]-dst[0,0])); h = int(cr(dst[2,1]-dst[0,1]))
expect = {"R":"00","G1":"01","G2":"10","B":"11"}
for ch, pl in expect.items():
    g = cv2.imread(f"{grid_dir}/iso{iso}_grid_{ch}.tif", cv2.IMREAD_UNCHANGED)
    p = cv2.imread(f"{bayer_dir}/iso{iso}_p{pl}.tif", cv2.IMREAD_UNCHANGED)
    Y, X = np.mgrid[y0:y0+h, x0:x0+w].astype(np.float64)
    den = k[6]*X + k[7]*Y + 1
    xu = cr((k[0]*X + k[1]*Y + k[2]) / den).astype(int); yu = cr((k[3]*X + k[4]*Y + k[5]) / den).astype(int)
    ref = p[yu, xu]
    same = (g.shape == ref.shape) and np.array_equal(g, ref)
    diff = int((g != ref).sum()) if g.shape == ref.shape else -1
    print(f"ISO {iso} {ch}: exported {g.shape} {g.dtype}; pixels different from the RAW at the mapped position: {diff} of {g.size} -> {'EXACT' if same else 'MISMATCH'}")
