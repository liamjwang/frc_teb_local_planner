import java.io.File;

class TebInterface {

    static {
        File lib = new File("../../../../lib/libjni_teb_local_planner.so");
        System.out.println(lib.getAbsolutePath());
        System.load(lib.getAbsolutePath());
    }

    public static native PoseSE2 plan(PoseSE2 start, PoseSE2 goal, PoseSE2 start_vel, boolean free_goal_vel);
}