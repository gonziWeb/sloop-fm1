/* (tests/songs_test.c) the flash: a RAM image of the 1 MiB NOR, as tests/storage_test.c has it */
static uint8_t flash_ok;
static int st_read(uint32_t off, void *dst, uint32_t n) { memcpy(dst, nor + off, n); return 0; }
static int st_erase(uint32_t off) { assert(!(off & 0xFFFu)); memset(nor + off, 0xFF, 4096); return 0; }
static int st_prog(uint32_t off, const void *src, uint32_t n)
{
    const uint8_t *s = src;
    uint32_t i;
    assert((off & 0xFFu) + n <= 256u);
    for (i = 0; i < n; i++)
        nor[off + i] &= s[i];
    return 0;
}
#define ST_XIP_PTR(off) (nor + (off))
#include "../firmware/src/storage.c"
static void up_boot(void) {}
static uint32_t irq_save(void) { return 0; }
static void irq_restore(uint32_t f) { (void)f; }
static uint32_t fl_jedec_ram(void) { return 0x856014u; }
#define FL_FAR(fn) (fn)
static void fl_plain_window_init(void) {}
#include "../firmware/src/project.c"

static int bad;
static int check(const char *what, int ok)
{
    printf("songs: %-58s %s\n", what, ok ? "ok" : "FAIL");
    bad += !ok;
    return ok;
}
static void settle(void) { transport_req = 0; song.playing = 0; }   /* (the audio ISR takes the stop) */
static void power_cycle(void)                         /* RAM gone (proj_slot is .noinit: zeroed here, a cold start) */
{
    memset(proj_slot, 0, sizeof proj_slot);
    song_bank = 0;
    arr_defaults(&arrangement);
    memset(&songs, 0, sizeof songs);
    memset(&songs_saved, 0, sizeof songs_saved);
    sec_dirty = song_dirty = 0;
    persist_boot();
    settle();
}
static int msg_is(const char *s) { return !strcmp(ui.msg, s); }

int main(void)
{
    arr_config_t a1, a2;
    memset(nor, 0xFF, sizeof nor);
    {   /* an FM-1 updated from SLOOP with a sample set in USR4: its header is still at 0xE7000 */
        smp_user_hdr_t h;
        memset(&h, 0, sizeof h);
        h.magic = SMP_USER_MAGIC, h.version = 1, h.nz = 1, h.data_len = 1024;
        h.zone[0].off = 0, h.zone[0].n = 100, h.zone[0].ls = 0, h.zone[0].le = 99;
        h.zone[0].rate = 1u << 16, h.zone[0].lo = 0, h.zone[0].hi = 127;
        memcpy(nor + 0xE7000u, &h, sizeof h);
    }
    host_tracks_init();
    power_cycle();
    check("updated from SLOOP: song 1, no song list yet", song_bank == 0 && flash_ok);
    check("... the old USR4 sample set is not played (its flash is songs 2..3 now)", usr_nz[3] == 0);
    memcpy(nor + 0xA0000u, nor + 0xE7000u, sizeof(smp_user_hdr_t));
    smp_user_scan(0);
    check("... (the same set in USR1 plays: the test's set is a valid one)", usr_nz[0] == 1);
    memset(nor + 0xA0000u, 0xFF, sizeof(smp_user_hdr_t));
    smp_user_scan(0);

    trk[0].p[P_REV] = 11;                             /* song 1: section A, its own order */
    section_store(0);
    arr_defaults(&a1);
    a1.count = 1, a1.entry[0].scene = 0, a1.entry[0].bars = 2;
    arrangement = a1;
    song_dirty = 1;
    sections_write();
    check("song 1 section A in flash where SLOOP keeps it (0x97000)",
          !memcmp(nor + 0x97000u, "FELU", 4) || !memcmp(nor + 0x98000u, "FELU", 4));

    song.playing = 1;
    song_switch(1);
    check("playing: STOP FIRST, song 1 stays", msg_is("STOP FIRST") && song_bank == 0);
    settle();

    song_switch(1);
    check("song 2: a new, empty song", song_bank == 1 && msg_is("NEW SONG 2") && !project_used(0) && !project_used(1));
    check("song 2: the default order", arrangement.count == 4);
    check("song 2: the loop stays as it was (still song 1's A)", trk[0].p[P_REV] == 11);

    trk[0].p[P_REV] = 22;                             /* song 2: sections A and B */
    section_store(0);
    trk[0].p[P_REV] = 23;
    section_store(1);
    arr_defaults(&a2);
    a2.count = 2, a2.entry[0].scene = 1, a2.entry[1].scene = 0, a2.entry[0].bars = a2.entry[1].bars = 1;
    arrangement = a2;
    song_dirty = 1;
    sections_write();
    check("song 2's sections in USR4's flash (0xE7000..)",
          !memcmp(nor + 0xE7000u, "FELU", 4) || !memcmp(nor + 0xE8000u, "FELU", 4));

    song_switch(0);
    settle();
    check("back to song 1: SONG 1, its section A is the loop", song_bank == 0 && msg_is("SONG 1") &&
          trk[0].p[P_REV] == 11 && project_used(0) && !project_used(1));
    check("song 1: its own order again", !memcmp(&arrangement, &a1, sizeof a1));

    song_switch(1);
    settle();
    check("song 2: A first (REV 22), B there too", song_bank == 1 && trk[0].p[P_REV] == 22 && project_used(1));
    check("song 2: its own order", !memcmp(&arrangement, &a2, sizeof a2));

    power_cycle();
    check("power off and on: still song 2, its sections, its order",
          song_bank == 1 && project_used(0) && project_used(1) && !memcmp(&arrangement, &a2, sizeof a2));

    trk[0].p[P_REV] = 24;                             /* a section stored, the FM-1 switched off before it was written */
    section_store(1);
    song_switch(2);
    settle();
    check("song 3: song 2's pending section written first", song_bank == 2 && msg_is("NEW SONG 3"));
    song_switch(1);
    settle();
    section_load(1);
    settle();
    check("... and it is there in song 2 (REV 24)", trk[0].p[P_REV] == 24);

    {   /* the SAVE layer: keys 9 / 10 / 11 are songs 1 / 2 / 3 */
        layer_key(LY_SONG, key_of_white(8), 1);
        settle();
        check("SAVE + key 9: song 1", song_bank == 0 && msg_is("SONG 1"));
        layer_key(LY_SONG, key_of_white(10), 1);
        settle();
        check("SAVE + key 11: song 3 (empty)", song_bank == 2 && msg_is("NEW SONG 3"));
        layer_key(LY_SONG, key_of_white(10), 1);
        check("SAVE + key 11 again: says SONG 3", song_bank == 2 && msg_is("SONG 3"));
    }
    {   /* project_erase and project_save work on the song in use only */
        song_switch(0);
        settle();
        project_erase(0);
        song_switch(1);
        settle();
        check("erasing song 1's A leaves song 2's A", project_used(0) && trk[0].p[P_REV] == 22);
        song_switch(0);
        settle();
        check("song 1: A gone", !project_used(0));
    }
    smp_user_scan(3);
    check("USR4 stays empty (its flash holds songs 2 and 3)", usr_nz[3] == 0);
    printf("%s\n", bad ? "SONGS TEST FAILED" : "songs test passed");
    return bad != 0;
}
